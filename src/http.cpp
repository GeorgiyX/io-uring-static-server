// project
#include "http.h"

// stl
#include <vector>
#include <string>
#include <cstring>


// skyr
#include <skyr/url.hpp>
#include <skyr/percent_encoding/percent_decode.hpp>

bool HTTPParser::parseMethod(const char *data, size_t length) {
    static std::vector<std::pair<Method, std::string>> methods {
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
                                 [data, length](const std::pair<Method, std::string> &method)-> bool {
        return method.second.length() <= length && !std::strncmp(method.second.c_str(), data, method.second.length());
    });

    if (iterator == methods.end()) {
        method = Method::ERR;
        return false;
    }

    method = iterator->first;
    return true;
}

HTTPParser::ParseStatus HTTPParser::parse(const char *data, size_t length) {
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
    return strstr(data, "HTTP/") && strstr(data, "\r\n");
}

void HTTPParser::parsePath(const char *data, size_t length) {
    auto start = strstr(data, " ") + 1;
    auto end = strstr(data, "?");
    end = end ? end : strstr(start, " ");
    auto pathLength = end - start;
    path = skyr::percent_decode(std::string_view(start, pathLength)).value();
}
