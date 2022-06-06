
#include "event_loop.h"


EventLoop::EventLoop(std::shared_ptr<io_uring> ring) :
_ring(std::move(ring)), _connection(_ring) {

}


void EventLoop::init(int fd) {
    _connection.restore(ConnectionState::packUserData(ConnectionState::INIT, fd), 0).process();
}

void EventLoop::loop() {

}

