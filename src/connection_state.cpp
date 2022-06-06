#include "connection_state.h"

void ConnectionState::process() {

}

ConnectionState::ConnectionState(std::shared_ptr<io_uring> _ring) :
_state(ACCEPT), _result(0), _fd(0), _ring(std::move(_ring)) {

}

void ConnectionState::addAcceptorSQE(int fd) {
    auto sqe = io_uring_get_sqe(_ring.get());
    io_uring_prep_accept(sqe, _fd, nullptr, nullptr, 0);
    io_uring_sqe_set_data(sqe, reinterpret_cast<void *>(ConnectionState::packUserData(ConnectionState::ACCEPT, fd)));
    io_uring_submit(_ring.get());
}

uint64_t ConnectionState::packUserData(ConnectionState::State state, int fd) {
    return static_cast<uint64_t>(state) << 32 | fd;
}

ConnectionState::State ConnectionState::unpackState(uint64_t packedData) {
    return static_cast<State>((packedData & (std::numeric_limits<unsigned long long>::max() - std::numeric_limits<unsigned int>::max())) >> 32);
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
