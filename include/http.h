#ifndef IO_URING_STATIC_SERVER_HTTP_H
#define IO_URING_STATIC_SERVER_HTTP_H

// stl
#include <stddef.h>
#include <string>


struct HTTPParser {
    enum ParseStatus {
        OK,
        INVALID,
        INCOMPLETE,
    };

    enum Method {
        OPTIONS,
        GET,
        HEAD,
        POST,
        PUT,
        PATCH,
        DELETE,
        TRACE,
        CONNECT,
        ERR
    };

    struct Parsed {
        const char *methodData;
        size_t methodLength;
        const char *pathData;
        size_t pathLength;
    };

    ParseStatus status;
    Method method;
    std::string path;

    ParseStatus parse(const char *data, size_t length);

private:
    bool parseMethod(const char *data, size_t length);
    bool hasFirstRow(const char *data, size_t length);
    void parsePath(const char *data, size_t length);

};



#endif //IO_URING_STATIC_SERVER_HTTP_H
