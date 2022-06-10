// project
#include "http.h"

// stl
#include <vector>
#include <string>
#include <cstring>

// skyr
#include <skyr/url.hpp>
#include <skyr/percent_encoding/percent_decode.hpp>

const size_t DATA_BUFFER_SIZE = 64;

bool HTTPParser::parseMethod(const char *data, size_t length) {
    static std::vector<std::pair<Method, std::string>> methods{
            std::make_pair(Method::OPTIONS, "OPTIONS"),
            std::make_pair(Method::GET, "GET"),
            std::make_pair(Method::HEAD, "HEAD"),
            std::make_pair(Method::POST, "POST"),
            std::make_pair(Method::PUT, "PUT"),
            std::make_pair(Method::PATCH, "PATCH"),
            std::make_pair(Method::DELETE, "DELETE"),
            std::make_pair(Method::TRACE, "TRACE"),
            std::make_pair(Method::CONNECT, "CONNECT"),
    };

    auto iterator = std::find_if(methods.begin(), methods.end(),
                                 [data, length](const std::pair<Method, std::string> &method) -> bool {
                                     return method.second.length() <= length &&
                                            std::strstr(data, method.second.c_str());
                                 });

    if (iterator == methods.end()) {
        method = Method::ERR;
        return false;
    }

    method = iterator->first;
    return true;
}

HTTPParser::ParseStatus HTTPParser::parse(char *data, size_t length) {
    if (!hasFirstRow(data, length)) {
        return HTTPParser::INCOMPLETE;
    }
    if (!parseMethod(data, length)) {
        return HTTPParser::INVALID;
    }
    parsePath(data, length);
    return HTTPParser::OK;
}

bool HTTPParser::hasFirstRow(const char *data, size_t length) {
    return strstr(data, "\r\n");
}

void HTTPParser::parsePath(char *data, size_t length) {
    auto eol = strstr(data, "\r\n");
    *eol = '\0';
    auto start = strstr(data, " /") + 2;
    auto end = strstr(start, "?");
    end = end ? end : strstr(start, " ");
    auto pathLength = end - start;
    path = skyr::percent_decode(std::string_view(start, pathLength)).value();
}

void HTTPResp::copyResponse(char *buffer, size_t &written, HTTPResp::RespCode code) {
    static std::unordered_map<RespCode, std::string> responseFormats{
            std::make_pair(RespCode::RESP_400, "HTTP/1.1 400 Bad Request\r\nServer: hl-server\r\n"
                                               "Date: %s\r\nConnection: close\r\n\r\n"),
            std::make_pair(RespCode::RESP_404, "HTTP/1.1 404 Not Found\r\nServer: hl-server\r\n"
                                               "Date: %s\r\nConnection: close\r\n\r\n"),
            std::make_pair(RespCode::RESP_403, "HTTP/1.1 403 Forbidden\r\nServer: hl-server\r\n"
                                               "Date: %s\r\nConnection: close\r\n\r\n"),
            std::make_pair(RespCode::RESP_413, "HTTP/1.1 413 Payload Too Large\r\nServer: hl-server\r\n"
                                               "Date: %s\r\nConnection: close\r\n\r\n"),
            std::make_pair(RespCode::RESP_405, "HTTP/1.1 405 Method Not Allowed\r\nAllow: GET, HEAD\r\n"
                                               "Server: hl-server\r\nDate: %s\r\nConnection: close\r\n\r\n"),
    };
    char date[DATA_BUFFER_SIZE];
    HTTPResp::writeDate(date, DATA_BUFFER_SIZE);
    written = std::snprintf(buffer, BufferManager::get_const_instance().bufferSize(),
                  responseFormats.at(code).c_str(), date);
}

void
HTTPResp::copyResponse200(char *buffer, size_t &written, const FileManager::FileInfo &file) {
    static std::string responseFormat = "HTTP/1.1 200 OK\r\nServer: hl-server\r\n"
                                        "Date: %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n"
                                        "Connection: close\r\n\r\n";
    char date[DATA_BUFFER_SIZE];
    HTTPResp::writeDate(date, DATA_BUFFER_SIZE);
    written = std::snprintf(buffer, BufferManager::get_const_instance().bufferSize(),
                  responseFormat.c_str(), date, HTTPResp::getMimeType(file.extension), file.size);
}

const char *HTTPResp::getMimeType(const std::string &extension) {
    static const std::unordered_map<std::string, std::string> responseFormats{
            std::make_pair(".jpg", "image/jpeg"),
            std::make_pair(".jpeg", "image/jpeg"),
            std::make_pair(".png", "image/png"),
            std::make_pair(".gif", "image/gif"),
            std::make_pair(".htm", "text/html"),
            std::make_pair(".html", "text/html"),
            std::make_pair(".js", "application/javascript"),
            std::make_pair(".css", "text/css"),
            std::make_pair(".txt", "text/plain"),
            std::make_pair(".swf", "application/x-shockwave-flash"),
    };
    static const std::string defaultMime = "application/octet-stream";
    if (!responseFormats.contains(extension)) {
        return defaultMime.c_str();
    }
    return responseFormats.at(extension).c_str();
}

void HTTPResp::writeDate(char *buffer,  size_t bufferLen) {
    static const std::string dateFormat = "%a, %d %b %Y %H:%M:%S GMT";
    time_t unixTime = time(nullptr);
    struct tm *gmTime = gmtime(&unixTime);
    strftime(buffer, bufferLen, dateFormat.c_str(), gmTime);
}
