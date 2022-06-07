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
                                    ", descriptor: " + std::to_string(_fd)));
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
    auto sqe = io_uring_get_sqe(_ring.get());
    io_uring_prep_accept(sqe, _fd, nullptr, nullptr, 0);
    io_uring_sqe_set_data(sqe, reinterpret_cast<void *>(ConnectionState::packUserData(ConnectionState::ACCEPT, fd)));
    io_uring_submit(_ring.get());
}

void ConnectionState::addReadSQE(int fd) {
    auto sqe = io_uring_get_sqe(_ring.get());
    size_t filling = _buffers.getBufferFilling(fd);
    _io_uring_prep_read(sqe, fd,
                        _buffers.getBuffer(fd) + filling,
                        _buffers.bufferSize() - filling);
    io_uring_sqe_set_data(
            sqe, reinterpret_cast<void *>(ConnectionState::packUserData(ConnectionState::RECEIVE, fd)));
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
    }

    off_t offset = 0;
    /* sendfile is synchronous, but very fast. works faster than via splice */
    if (sendfile(_fd, file->fd, &offset, file->size) < 0) {
        Logger::warning("error in sendfile");
    }

    close(_fd);
}

void ConnectionState::processReceive() {
    /* zero send */
    if (!_result) { return; }
    //todo: pase http
}
