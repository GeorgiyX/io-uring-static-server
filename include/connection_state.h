#ifndef IO_URING_STATIC_SERVER_CONNECTION_STATE_H
#define IO_URING_STATIC_SERVER_CONNECTION_STATE_H

// stl
#include <cstdint>
#include <memory>
#include <liburing.h>

class ConnectionState {
public:
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
    void addAcceptorSQE(int fd);
    void readSQE();
    void sendSQE();

    State _state;
    int _result;
    int _fd;
    std::shared_ptr<io_uring> _ring;
};

#endif //IO_URING_STATIC_SERVER_CONNECTION_STATE_H
