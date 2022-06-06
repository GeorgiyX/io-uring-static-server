#ifndef IO_URING_STATIC_SERVER_SERVER_H
#define IO_URING_STATIC_SERVER_SERVER_H

// stl
#include <vector>
#include <memory>

// uring
#include <liburing.h>

class Server {
public:
    explicit Server(const std::string &configPath);
    void run();
private:
    static std::shared_ptr<io_uring> createRing(int backendFd = -1);
    void registerBuffers();

    std::vector<std::shared_ptr<io_uring>> _rings;
    std::shared_ptr<io_uring> _mainRing;
};


#endif //IO_URING_STATIC_SERVER_SERVER_H
