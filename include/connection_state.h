#ifndef IO_URING_STATIC_SERVER_CONNECTION_STATE_H
#define IO_URING_STATIC_SERVER_CONNECTION_STATE_H

// project
#include "resources.h"
#include "http.h"

// stl
#include <cstdint>
#include <memory>
#include <liburing.h>

class ConnectionState {
public:
    using io_uring_prep_write_ptr =  std::function<void(io_uring_sqe *, int, const void *, unsigned)>;
    using io_uring_prep_read_ptr =  std::function<void(io_uring_sqe *, int, void *, unsigned)>;

    enum State : int {
        ACCEPT = 0,
        RECEIVE,
        WRITE,
        INIT,
    };

    explicit ConnectionState(std::shared_ptr<io_uring> ring);

    static uint64_t packUserData(State state, int fd);
    static State unpackState(uint64_t packedData);
    static int unpackFd(uint64_t packedData);


    ConnectionState &restore(uint64_t packedData, int result);
    void process();



private:
    void processAccept();
    void processWrite();
    void processReceive();

    void processOkParse();
    void processIncompleteParse();

    void addAcceptorSQE(int fd);
    void addReadSQE(int fd);
    void addSendSQE(int fd);
    void sendOnlyHeaders(HTTPResp::RespCode code);
    void sendHeadersOk(const std::optional<FileManager::file_info_ref> &file, bool hasBody = true);

    State _state;
    int _result;
    int _fd;
    int _maxConnections;  // highest descriptor (sock + file)
    std::vector<std::optional<FileManager::file_info_ref> > _activeFileTransmit;  // map: fd-file
    std::shared_ptr<io_uring> _ring;
    BufferManager &_buffers;
    FileManager &_files;
    io_uring_prep_read_ptr _io_uring_prep_read;
    io_uring_prep_write_ptr _io_uring_prep_write;
    HTTPParser _parser;

};

#endif //IO_URING_STATIC_SERVER_CONNECTION_STATE_H
