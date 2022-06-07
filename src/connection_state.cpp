// project
#include "connection_state.h"
#include "logger.h"

void ConnectionState::process() {
    if (_result < 0) {
        Logger::warning(std::string("unsuccessful request, state: " +
                                    std::to_string(static_cast<int>(_state)) +
                                    ", descriptor: " + std::to_string(_fd)));

        return;
    }


    switch (_state) {
        case ACCEPT:
            break;
        case RECEIVE:
            break;
        case WRITE:
            break;
        case INIT:
            addAcceptorSQE(_fd);
            break;
    }

    switch (request_data_event_type(cqe->user_data)) {
        case EVENT_TYPE_ACCEPT:
            /* После приеме содениения нужно снова попросить ядро следить за accept сокетом*/
            add_accept_request(ring, server_fd,
                               &client_addr, &client_addr_len);
            if (LIKELY(cqe->res < nconnections)) {
                /* Для этого клиента ничего еще не прочиатно буффер */
                buffer_lengths[cqe->res] = 0;
                add_read_request(ring, cqe->res);
            } else {   /* Соединений больше чем мы установили в конфиге, см. порядок нумерации дескрипторов */
                fprintf(stderr, "server capacity exceeded: %d / %d\n",
                        cqe->res, nconnections);
                close(cqe->res);
            }
            break;

        case EVENT_TYPE_READ:
            /* recv может иногда вернуть 0 байт, это валидная ситуация и мы ничего не делаем */
            if (LIKELY(cqe->res)) // non-empty request?
                handle_request(ring,
                               request_data_client_fd(cqe->user_data),
                               cqe->res);
            break;

        case EVENT_TYPE_WRITE: {
            int client_fd = request_data_client_fd(cqe->user_data);
            if (LIKELY(file_fds[client_fd] != 0)) {
                off_t offset = 0;
                /* sendfile синхронный, но очень быстрый. быстрее работает чем через splice*/
                if (sendfile(client_fd, file_fds[client_fd],
                             &offset, buffer_lengths[client_fd]) < 0)
                    perror("sendfile");
            }
            close(client_fd);
        }
    }

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
