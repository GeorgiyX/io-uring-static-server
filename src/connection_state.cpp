// project
#include "connection_state.h"
#include "logger.h"
#include "config.h"
#include "io_uring_wrappers.h"
#include "http.h"

// stl
#include <sys/sendfile.h>

ConnectionState::ConnectionState(std::shared_ptr<io_uring> _ring) :
        _state(ACCEPT), _result(0), _fd(0), _ring(std::move(_ring)),
        _maxConnections(0), _activeFileTransmit(),
        _buffers(BufferManager::get_mutable_instance()),
        _files(FileManager::get_mutable_instance()),
        _io_uring_prep_read(), _io_uring_prep_write(), _parser() {
    const auto &params = Config::get_const_instance().params();
    if (params.registerBuffers) {
        _io_uring_prep_write = io_uring_prep_write_fixed_wrapped;
        _io_uring_prep_read = io_uring_prep_read_fixed_wrapped;
    } else {
        _io_uring_prep_write = io_uring_prep_send_wrapped;
        _io_uring_prep_read = io_uring_prep_recv_wrapped;
    }
    _maxConnections = params.ringEntities + params.rlimitNoFile;
    _activeFileTransmit.resize(_maxConnections);
}

void ConnectionState::process() {
    if (_result < 0) {
        Logger::warning(std::string("unsuccessful request, state: " +
                                    std::to_string(static_cast<int>(_state)) +
                                    ", descriptor: " + std::to_string(_fd) +
                                    ", result: " + std::to_string(_result)));
        return;
    }

    switch (_state) {
        case ACCEPT:
            processAccept();
            break;
        case RECEIVE:
            processReceive();
            break;
        case WRITE:
            processWrite();
            break;
        case INIT:
            addAcceptorSQE(_fd);
            break;
    }
}

uint64_t ConnectionState::packUserData(ConnectionState::State state, int fd) {
    return static_cast<uint64_t>(state) << 32 | fd;
}

ConnectionState::State ConnectionState::unpackState(uint64_t packedData) {
    return static_cast<State>(
            (packedData & (std::numeric_limits<unsigned long long>::max() - std::numeric_limits<unsigned int>::max()))
                    >> 32);
}

int ConnectionState::unpackFd(uint64_t packedData) {
    return packedData & std::numeric_limits<unsigned int>::max();
}

ConnectionState &ConnectionState::restore(uint64_t packedData, int result) {
    _state = ConnectionState::unpackState(packedData);
    _fd = ConnectionState::unpackFd(packedData);
    _result = result;
    return *this;
}

void ConnectionState::addAcceptorSQE(int fd) {
    Logger::info("addAcceptorSQE: " + std::to_string(fd));
    auto sqe = io_uring_get_sqe(_ring.get());
    io_uring_prep_accept(sqe, _fd, nullptr, nullptr, 0);
    io_uring_sqe_set_data(sqe, reinterpret_cast<void *>(ConnectionState::packUserData(ConnectionState::ACCEPT, fd)));
    io_uring_submit(_ring.get());
}

void ConnectionState::addReadSQE(int fd) {
    Logger::info("addReadSQE: " + std::to_string(fd));
    auto sqe = io_uring_get_sqe(_ring.get());
    size_t filling = _buffers.getBufferFilling(fd);
    _io_uring_prep_read(sqe, fd,
                        _buffers.getBuffer(fd) + filling,
                        _buffers.bufferSize() - filling);
    io_uring_sqe_set_data(
            sqe, reinterpret_cast<void *>(ConnectionState::packUserData(ConnectionState::RECEIVE, fd)));
    io_uring_submit(_ring.get());
}


void ConnectionState::addSendSQE(int fd) {
    Logger::info("addSendSQE: " + std::to_string(fd));
    struct io_uring_sqe *sqe = io_uring_get_sqe(_ring.get());
    auto buf = _buffers.getBuffer(fd);
    std:: cout << _buffers.getBufferFilling(fd) <<  std::endl;
    _io_uring_prep_write(sqe, fd, _buffers.getBuffer(fd), _buffers.getBufferFilling(fd));
    io_uring_sqe_set_data(
            sqe, reinterpret_cast<void *>(ConnectionState::packUserData(ConnectionState::WRITE, fd)));
    io_uring_submit(_ring.get());
}

void ConnectionState::processAccept() {
    addAcceptorSQE(_fd);
    if (_result > _maxConnections) {
        Logger::warning("descriptor number (" + std::to_string(_result)
                        + ") signals that the number of connections is exceeded, the connection will be closed");
        close(_result);
        return;
    }
    _buffers.setBufferFilling(0, _result);
    addReadSQE(_result);
}

void ConnectionState::processWrite() {
    /* if there is no value, then sending the file is not required. */
    auto file = _activeFileTransmit.at(_fd);
    if (!file) {
        close(_fd);
        return;
    }

    off_t offset = 0;
    /* sendfile is synchronous, but very fast. works faster than via splice + pipe */
    if (sendfile(_fd, file.value().get().fd, &offset, file.value().get().size) < 0) {
        Logger::warning(std::string("error in sendfile: ")  + strerror(errno));
    }

    close(_fd);
}

void ConnectionState::processReceive() {
    /* zero send */
    if (!_result) { return; }

    auto bufferLength = _buffers.getBufferFilling(_fd) + _result;
    _buffers.setBufferFilling(bufferLength, _fd);

    auto parseResult = _parser.parse(_buffers.getBuffer(_fd), bufferLength);
    switch (parseResult) {
        case HTTPParser::OK:
            processOkParse();
            break;
        case HTTPParser::INVALID:
            sendOnlyHeaders(HTTPResp::RESP_400);
            break;
        case HTTPParser::INCOMPLETE:
            processIncompleteParse();
            break;
    }
}

void ConnectionState::processIncompleteParse() {
    if (_buffers.getBufferFilling(_fd) < _buffers.bufferSize()) {
        addReadSQE(_fd);
        return;
    }
    sendOnlyHeaders(HTTPResp::RESP_413);
}

void ConnectionState::processOkParse() {
    if (_parser.method != HTTPParser::HEAD && _parser.method != HTTPParser::GET) {
        sendOnlyHeaders(HTTPResp::RESP_405);
        return;
    }

    if (_parser.path.find("../") != std::string::npos) {
        sendOnlyHeaders(HTTPResp::RESP_403);
        return;
    }

    auto file = _files.getFileInfo(_parser.path);
    if (!file) {
        sendOnlyHeaders(HTTPResp::RESP_404);
        return;
    }

    sendHeadersOk(file->get());
}

void ConnectionState::sendOnlyHeaders(HTTPResp::RespCode code) {
    size_t respLength = 0;
    _activeFileTransmit.at(_fd) = std::nullopt;
    HTTPResp::copyResponse(_buffers.getBuffer(_fd), respLength, code);
    _buffers.setBufferFilling(respLength, _fd);
    addSendSQE(_fd);
}

void ConnectionState::sendHeadersOk(const std::optional<FileManager::file_info_ref>  &file) {
    size_t respLength = 0;
    _activeFileTransmit.at(_fd) = file;
    HTTPResp::copyResponse200(_buffers.getBuffer(_fd), respLength, file->get());
    _buffers.setBufferFilling(respLength, _fd);
    addSendSQE(_fd);
}
