#ifndef IO_URING_STATIC_SERVER_EVENT_H
#define IO_URING_STATIC_SERVER_EVENT_H

// stl
#include <cstdint>

class HTTPSession {
    enum State {
        ACCEPT = 0,
        RECEIVE,
        WRITE,
    };

public:
    HTTPSession &restore(uint64_t packedData);
    void process();

private:


};

#endif //IO_URING_STATIC_SERVER_EVENT_H
