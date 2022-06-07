#ifndef IO_URING_STATIC_SERVER_IO_URING_WRAPPERS_H
#define IO_URING_STATIC_SERVER_IO_URING_WRAPPERS_H

// liburing
#include <liburing.h>

static inline void io_uring_prep_send_wrapped(io_uring_sqe *sqe, int sockfd, const void *buf, unsigned len) {
    io_uring_prep_rw(IORING_OP_SEND, sqe, sockfd, buf,  len, 0);
    sqe->msg_flags = 0;
}


static inline void io_uring_prep_write_fixed_wrapped(io_uring_sqe *sqe, int sockfd, const void *buf, unsigned len) {
    io_uring_prep_rw(IORING_OP_WRITE_FIXED, sqe, sockfd, buf, len, 0);
    sqe->buf_index = 0;
}

static inline void io_uring_prep_recv_wrapped(struct io_uring_sqe *sqe, int sockfd, void *buf, unsigned len)
{
    io_uring_prep_rw(IORING_OP_RECV, sqe, sockfd, buf, len, 0);
    sqe->msg_flags = 0;
}

static inline void io_uring_prep_read_fixed_wrapped(struct io_uring_sqe *sqe, int sockfd, void *buf, unsigned len)
{
    io_uring_prep_rw(IORING_OP_READ_FIXED, sqe, sockfd, buf, len, 0);
    sqe->buf_index = 0;
}

#endif //IO_URING_STATIC_SERVER_IO_URING_WRAPPERS_H
