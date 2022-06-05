#include <stdexcept>
#include <boost/format.hpp>
#include <iostream>

#include <liburing.h>
#include <linux/io_uring.h>
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <cassert>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>
#include <thread>
#include <functional>

#define MAX_CONNECTIONS 4096
#define BACKLOG 512
#define MAX_MESSAGE_LEN 2048
#define IORING_FEAT_FAST_POLL (1U << 5)
#define ANSWER "HTTP/1.0 200 OK\r\nServer: otus-io-uring\r\nDate: 2022.06.05\r\n\ Content-Type: application/octet-stream\r\nContent-Length: 2\r\n\r\nhi"

void add_accept(struct io_uring *ring, int fd, struct sockaddr *client_addr, socklen_t *client_len);

void add_socket_read(struct io_uring *ring, int fd, size_t size);

void add_socket_write(struct io_uring *ring, int fd, size_t size);

/**
 * Каждое активное соединение в нашем приложение описывается структурой conn_info.
 * fd - файловый дескриптор сокета.
 * type - описывает состояние в котором находится сокет - ждет accept, read или write.
 */
typedef struct conn_info {
    int fd;
    unsigned type;
} conn_info;

enum {
    ACCEPT,
    READ,
    WRITE,
};

// Буфер для соединений.
conn_info conns[MAX_CONNECTIONS];

// Для каждого возможного соединения инициализируем буфер для чтения/записи.
char bufs[MAX_CONNECTIONS][MAX_MESSAGE_LEN];


int crete_acceptor(int port) {
    sockaddr_in serv_addr{};
    socklen_t client_len = sizeof(sockaddr_in);

    int sock_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    const int val = 1;
    setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    assert(bind(sock_listen_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0);
    assert(listen(sock_listen_fd, BACKLOG) >= 0);

    return sock_listen_fd;
}

struct URingLoop {

    io_uring _ring;
    int _acceptorFd;
    std::string suffix;

    explicit URingLoop(int acceptorFd, int workQueueFd = 0) : _acceptorFd(acceptorFd) {
        io_uring_params params{};
        memset(&params, 0, sizeof(params));
        params.flags = IORING_SETUP_SQPOLL | IORING_SETUP_SQ_AFF;

        if (workQueueFd) {
            params.wq_fd = workQueueFd;
            params.flags |= IORING_SETUP_ATTACH_WQ;
        }

        if (io_uring_queue_init_params(10000, &_ring, &params) < 0) {
            throw std::runtime_error("can't create io_uring");
        }
        std::cout << "thread: " << std::this_thread::get_id() << " ; ring fd: " << _ring.ring_fd << "; wq fd: " << params.wq_fd << std::endl;

        /**
         * Проверяем наличие фичи IORING_FEAT_FAST_POLL.
         * Для нас это наиболее "перформящая" фича в данном приложении,
         * фактически это встроенный в io_uring движок для поллинга I/O.
         */
        if (!(params.features & IORING_FEAT_FAST_POLL)) {
            printf("IORING_FEAT_FAST_POLL not available in the kernel, quiting...\n");
            exit(0);
        }
    }

    void addWrite(int fd, size_t size) {
        // Получаем указатель на первый доступный SQE.
        struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
//        std::cout << suffix << std::endl;
        // Хелпер io_uring_prep_send помещает в SQE операцию SEND, запись производится из буфера соответствующего клиентскому сокету.
//        std::memcpy(&bufs[fd] + size, suffix.c_str(), suffix.length() + 1);
        std::memcpy(&bufs[fd], ANSWER, strlen(ANSWER));
        io_uring_prep_send(sqe, fd, &bufs[fd], strlen(ANSWER), 0);

        // Устанавливаем состояние клиентского сокета в WRITE.
        conn_info *conn_i = &conns[fd];
        conn_i->fd = fd;
        conn_i->type = WRITE;

        // Устанавливаем в поле user_data указатель на socketInfo соответсвующий клиентскому сокету.
        io_uring_sqe_set_data(sqe, conn_i);
    }

    void addRead(int fd, size_t size) {
        // Получаем указатель на первый доступный SQE.
        struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
        // Хелпер io_uring_prep_recv помещает в SQE операцию RECV, чтение производится в буфер соответствующий клиентскому сокету.
        io_uring_prep_recv(sqe, fd, &bufs[fd], size, 0);

        // Устанавливаем состояние клиентского сокета в READ.
        conn_info *conn_i = &conns[fd];
        conn_i->fd = fd;
        conn_i->type = READ;

        // Устанавливаем в поле user_data указатель на socketInfo соответствующий клиентскому сокету.
        io_uring_sqe_set_data(sqe, conn_i);
    }

    void addAccept(int fd, struct sockaddr *client_addr, socklen_t *client_len) {
        // Получаем указатель на первый доступный SQE.
        struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
        // Хелпер io_uring_prep_accept помещает в SQE операцию ACCEPT.
        io_uring_prep_accept(sqe, fd, client_addr, client_len, 0);

        // Устанавливаем состояние серверного сокета в ACCEPT.
        conn_info *conn_i = &conns[fd];
        conn_i->fd = fd;
        conn_i->type = ACCEPT;

        // Устанавливаем в поле user_data указатель на socketInfo соответствующий серверному сокету.
        io_uring_sqe_set_data(sqe, conn_i);
    }

    void loop() {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        suffix = (boost::format(" [current thread id: %1%]") % std::this_thread::get_id()).str();
        std::cout << "start: " << suffix << std::endl;

        while (1) {
            struct io_uring_cqe *cqe;
            int ret;

            /**
             * Сабмитим все SQE которые были добавлены на предыдущей итерации.
             */
            io_uring_submit(&_ring);

            /**
             * Ждем когда в CQ буфере появится хотя бы одно CQE.
             */
            ret = io_uring_wait_cqe(&_ring, &cqe);
            assert(ret == 0);

            /**
             * Положим все "готовые" CQE в буфер cqes.
             */
            struct io_uring_cqe *cqes[BACKLOG];
            int cqe_count = io_uring_peek_batch_cqe(&_ring, cqes, sizeof(cqes) / sizeof(cqes[0]));

            for (int i = 0; i < cqe_count; ++i) {
                cqe = cqes[i];

                /**
                 * В поле user_data мы заранее положили указатель структуру
                 * в которой находится служебная информация по сокету.
                 */
                auto *user_data = (struct conn_info *) io_uring_cqe_get_data(cqe);

                /**
                 * Используя тип идентифицируем операцию к которой относится CQE (accept/recv/send).
                 */
                unsigned type = user_data->type;
                if (type == ACCEPT) {
                    int sock_conn_fd = cqe->res;

                    /**
                    * Если появилось новое соединение: добавляем в SQ операцию recv - читаем из клиентского сокета,
                    * продолжаем слушать серверный сокет.
                    */
                    addRead(sock_conn_fd, MAX_MESSAGE_LEN - 100);
                    addAccept(_acceptorFd, (struct sockaddr *) &client_addr, &client_len);
                } else if (type == READ) {
                    int bytes_read = cqe->res;

                    /**
                     * В случае чтения из клиентского сокета:
                     * если прочитали 0 байт - закрываем сокет
                     * если чтение успешно: добавляем в SQ операцию send - пересылаем прочитанные данные обратно, на клиент.
                     */
                    if (bytes_read <= 0) {
                        shutdown(user_data->fd, SHUT_RDWR);
                    } else {
                        addWrite(user_data->fd, bytes_read);
                    }
                } else if (type == WRITE) {
                    /**
                    * Запись в клиентский сокет окончена: добавляем в SQ операцию recv - читаем из клиентского сокета.
                    */
                    addRead(user_data->fd, MAX_MESSAGE_LEN);
                }

                io_uring_cqe_seen(&_ring, cqe);
            }
        }

    }
};

int main(int argc, char *argv[]) {
    std::cout << "main thread : " << (boost::format(" [current thread id: %1%]") % std::this_thread::get_id()).str() << std::endl;
    /**
     * Создаем серверный сокет и начинаем прослушивать порт.
     * Обратите внимание что при создании сокета мы НЕ УСТАНАВЛИВАЕМ флаг O_NON_BLOCK,
     * но при этом все чтения и записи не будут блокировать приложение.
     * Происходит это потому, что io_uring спокойно превращает операции над блокирующими сокетами в non-block системные вызовы.
     */
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    int acceptor = crete_acceptor(5000);

    std::vector<std::pair<std::shared_ptr<URingLoop>, std::shared_ptr<std::thread>>> loops;
    URingLoop mainLoop(acceptor, 0);
    mainLoop.addAccept(acceptor, (struct sockaddr *) &client_addr, &client_len);
    auto threadCount = 0;
    for (int i = 0; i < threadCount; i++) {
        auto loop = std::make_shared<URingLoop>(acceptor, mainLoop._ring.ring_fd);
        loop->addAccept(acceptor, (struct sockaddr *) &client_addr, &client_len);
        auto thread = std::make_shared<std::thread>(std::bind(&URingLoop::loop, loop.get()));
        loops.emplace_back(std::make_pair(
                std::move(loop),
                std::move(thread)
                ));
    }
    mainLoop.loop();
}
