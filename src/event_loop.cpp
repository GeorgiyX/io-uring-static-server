// project
#include "event_loop.h"
#include "logger.h"


EventLoop::EventLoop(std::shared_ptr<io_uring> ring) :
        _ring(std::move(ring)), _connection(_ring) {
}

void EventLoop::init(int fd) {
    _connection.restore(ConnectionState::packUserData(ConnectionState::INIT, fd), 0).process();
}

void EventLoop::loop() {
    while (true) {
        struct io_uring_cqe *cqe;
        if (io_uring_wait_cqe(_ring.get(), &cqe) < 0) {
            throw std::runtime_error("error in io_uring_wait_cqe");
        }
        _connection.restore(cqe->user_data, cqe->res);
        _connection.process();
        io_uring_cqe_seen(_ring.get(), cqe);
    }
}

