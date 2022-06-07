#ifndef IO_URING_STATIC_SERVER_HTTP_H
#define IO_URING_STATIC_SERVER_HTTP_H

#include <stddef.h>

struct HTTPParser {
    enum ParseStatus {
        OK,
        WRONG,
        INCOMPLETE,
    };

    struct Parsed {
        const char *methodData;
        size_t methodLength;
        const char *pathData;
        size_t pathLength;
    };

    ParseStatus status;

    ParseStatus parse(const char *data, size_t length);

private:

};

#endif //IO_URING_STATIC_SERVER_HTTP_H
