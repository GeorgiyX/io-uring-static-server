#include <skyr/url.hpp>
#include <skyr/percent_encoding/percent_decode.hpp>

#include <iostream>
#include <cstring>

int main() {
    std::string path = "GET https://translate.yandex.ru/lang=en-ru&text=Operation%20not%20pe@r!mitted HTTP/1.0";
    auto start = strstr(path.data(), " ") + 1;
    auto end = strstr(path.data(), "?");
    end = end ? end : strstr(start, " ");
    auto pathLength = end - start;

    path = skyr::percent_decode(std::string_view(start, pathLength)).value();
    std::cout << path << std::endl;
    return 0;
}