// project
#include "server.h"

// stl
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc > 2) {
        std::cout << "Usage: " << argv[0] << " ./path/to/config.conf" << std::endl;
        return 0;
    }

    std::string configPath = argc == 1 ? "/etc/httpd.conf/io-uring-server.conf" : argv[1];

    try {
        Server server(configPath);
        server.run();
    } catch (const std::runtime_error &err) {
        std::cerr << "error: " << err.what() << std::endl;
    }

    return 0;
}