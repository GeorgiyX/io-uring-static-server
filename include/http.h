#ifndef IO_URING_STATIC_SERVER_HTTP_H
#define IO_URING_STATIC_SERVER_HTTP_H

// project
#include "resources.h"

// stl
#include <stddef.h>
#include <string>


namespace HTTPResp {
    enum RespCode {
        RESP_413,
        RESP_400,
        RESP_405,
        RESP_403,
        RESP_404,
    };

    void copyResponse(char *buffer, size_t &written, RespCode code);
    void copyResponse200(char *buffer, size_t &written, const FileManager::FileInfo &file);
    const char * getMimeType(const std::string &extension);
    void writeDate(char *buffer, size_t bufferLen);
}

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

    ParseStatus parse(char *data, size_t length);

private:
    bool parseMethod(const char *data, size_t length);
    bool hasFirstRow(const char *data, size_t length);
    void parsePath(char *data, size_t length);

};



#endif //IO_URING_STATIC_SERVER_HTTP_H
