#ifndef IO_URING_STATIC_SERVER_EVENT_LOOP_H
#define IO_URING_STATIC_SERVER_EVENT_LOOP_H

// project
#include "connection_state.h"

// stl
#include <memory>

// uring
#include <liburing.h>

class EventLoop {
public:
    explicit EventLoop(std::shared_ptr<io_uring> ring);
    void loop();
    void init(int fd);

private:
    std::shared_ptr<io_uring> _ring;
    ConnectionState _connection;
};


#endif //IO_URING_STATIC_SERVER_EVENT_LOOP_H
