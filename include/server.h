#ifndef IO_URING_STATIC_SERVER_SERVER_H
#define IO_URING_STATIC_SERVER_SERVER_H

// stl
#include <vector>
#include <memory>

// uring
#include <liburing.h>

class Server {
public:
    void run();
private:
    std::vector<std::shared_ptr<io_uring>> _rings;
};


#endif //IO_URING_STATIC_SERVER_SERVER_H
