#include <liburing.h>
#include <cassert>
#include <unistd.h>
#include <cstring>

int main() {
    io_uring_params params{};
    io_uring ring{};
    memset(&params, 0, sizeof(params));

    /**
     * Создаем инстанс io_uring, не используем никаких кастомных опций.
     * Емкость SQ и CQ буфера указываем как 4096 вхождений.
     */
    int ret = io_uring_queue_init_params(4, &ring, &params);
    assert(ret == 0);

    char hello[] = "hello world!\n";

    // Добавляем операцию write в очередь SQ.
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_write(sqe, STDOUT_FILENO, hello, 13, 0);

    // Сообщаем io_uring о новых SQE в SQ.
    io_uring_submit(&ring);

    // Ждем пока в CQ появится новое CQE.
    struct io_uring_cqe *cqe;
    ret = io_uring_wait_cqe(&ring, &cqe);
    assert(ret == 0);

    // Проверяем отсутствие ошибок.
    assert(cqe->res > 0);

    // Dequeue из очереди CQ.
    io_uring_cqe_seen(&ring, cqe);

    io_uring_queue_exit(&ring);

    return 0;
}