// project
#include "server.h"

// stl
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " ./path/to/config.conf" << std::endl;
        return 0;
    }
    Server server(argv[1]);
    server.run();
    return 0;
}