#ifndef IO_URING_STATIC_SERVER_EVENT_LOOP_H
#define IO_URING_STATIC_SERVER_EVENT_LOOP_H

// stl
#include <memory>

// uring
#include <liburing.h>

class EventLoop {
public:
    EventLoop(std::shared_ptr<io_uring> ring);

    void loop();

    void addAcceptorSQE(int fd);

private:
    std::shared_ptr<io_uring> _ring;

};


#endif //IO_URING_STATIC_SERVER_EVENT_LOOP_H
