#ifndef IO_URING_STATIC_SERVER_SERVER_H
#define IO_URING_STATIC_SERVER_SERVER_H

// project
#include "event_loop.h"

// stl
#include <vector>
#include <memory>
#include <thread>

// uring
#include <liburing.h>

class Server {
public:
    explicit Server(const std::string &configPath);
    ~Server();
    void run();
private:
    static std::shared_ptr<io_uring> createRing(int backendFd = -1);
    static int createAcceptor(unsigned port, unsigned backlog);
    static void shutdownAcceptor(int fd);
    void registerBuffers();

    std::vector<std::shared_ptr<io_uring>> _rings;
    std::shared_ptr<io_uring> _mainRing;
    std::vector<std::shared_ptr<EventLoop>> _loops;
    std::shared_ptr<EventLoop> _mainLoop;
    std::vector<std::shared_ptr<std::thread>> _loopsThreads;
    int _acceptorFd;

};


#endif //IO_URING_STATIC_SERVER_SERVER_H
