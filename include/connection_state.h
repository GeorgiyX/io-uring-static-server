#ifndef IO_URING_STATIC_SERVER_HTTP_SESSION_H
#define IO_URING_STATIC_SERVER_HTTP_SESSION_H

// stl
#include <cstdint>

class HTTPSession {
public:
    enum State : int {
        ACCEPT = 0,
        RECEIVE,
        WRITE,
    };


    static uint64_t packUserData(State state, int fd);
    static State unpackState(uint64_t packedData);
    static int unpackFd(uint64_t packedData);


    HTTPSession &restore(uint64_t packedData);
    void process();

private:
    State _state;
    int result;
    int fs;
};

#endif //IO_URING_STATIC_SERVER_HTTP_SESSION_H
