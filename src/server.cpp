// project
#include "server.h"
#include "config.h"
#include "resources.h"
#include "utils.h"
#include "logger.h"

// stl
#include <cstring>
#include <algorithm>
#include <sys/resource.h>
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>

Server::Server(const std::string &configPath) :
_rings(), _mainRing(), _loops(), _mainLoop(), _acceptorFd(-1), _loopsThreads() {
    const auto &params = Config::get_mutable_instance().initFromFile(configPath).params();

    Logger::info("creating io_rings...");
    _mainRing = Server::createRing();
    for (size_t index = 0; index < params.cpuLimit; index++) {
        _rings.push_back(Server::createRing(_mainRing->ring_fd));
    }

    Logger::info(" done!\nallocating buffers...");
    BufferManager::get_mutable_instance().create(params.bufferSize, params.ringEntities);
    if (params.registerBuffers) {
        Logger::info(" done!\nregistering buffers...");
        registerBuffers();
    }

    Logger::info(" done!\nsetup storage...");
    FileManager::get_mutable_instance();


    Logger::info(" done!\ncreating event loops...");
    _mainLoop = std::make_shared<EventLoop>(_mainRing);
    std::for_each(_rings.begin(), _rings.end(), [this](auto &ring) {
        _loops.push_back(std::make_shared<EventLoop>(ring));
    });
    Logger::info(" done!");
}

std::shared_ptr<io_uring> Server::createRing(int backendFd) {
    io_uring_params params{};
    std::memset(&params, 0, sizeof(io_uring_params));

    if (Config::get_const_instance().params().enableSqPoll) {
        if (Utils::KernelVersion::current() < Utils::KernelVersion::serverMinimal()) {
            throw std::runtime_error("current kernel version is below the minimum allowed for using SQPOLL: " +
                                     std::string(Utils::KernelVersion::current()) + " < " +
                                     std::string(Utils::KernelVersion::serverMinimal()));
        }
        params.flags = IORING_SETUP_SQPOLL | IORING_SETUP_SQ_AFF;
        params.sq_thread_idle = std::numeric_limits<uint32_t>::max();
    }

    if (Config::get_const_instance().params().shareRingBackend && backendFd >= 0) {
        params.flags |= IORING_SETUP_ATTACH_WQ;
        params.wq_fd = backendFd;
    }

    auto ring = std::make_shared<io_uring>();
    if (io_uring_queue_init_params(Config::get_const_instance().params().ringEntities,
                                   ring.get(), &params) < 0) {
        throw std::runtime_error("can't create io_uring");
    }

    if (!(params.features & IORING_FEAT_FAST_POLL)) {
        std::cerr << "IORING_FEAT_FAST_POLL not available" << std::endl;
    }

    return ring;
}

void Server::registerBuffers() {
    std::vector<iovec> iovectors(BufferManager::buffersCount());
    for (int index = 0; index < iovectors.size(); index++) {
        iovectors[index].iov_base = BufferManager::get_mutable_instance().getBufferByFd(index);
        iovectors[index].iov_len = BufferManager::fullBufferSize();
    }

    Utils::increaseResourceLimit(RLIMIT_MEMLOCK, BufferManager::fullBufferSize());

    if (io_uring_register_buffers(_mainRing.get(), iovectors.data(), iovectors.size())) {
        throw std::runtime_error("can't register buffers");
    }
}

void Server::run() {
    Logger::info("open acceptor socket...");
    _acceptorFd = Server::createAcceptor(Config::get_const_instance().params().port,
                                         Config::get_const_instance().params().ringEntities);
    Logger::info(" done!\nstart event loops");
    _loopsThreads.reserve(_loops.size());
    std::for_each(_loops.begin(), _loops.end(), [this](std::shared_ptr<EventLoop> &loop) {
        loop->init(_acceptorFd);
        _loopsThreads.push_back(std::make_shared<std::thread>(std::bind(&EventLoop::loop, loop.get())));
    });
    _mainLoop->init(_acceptorFd);
    _mainLoop->loop();
}

int Server::createAcceptor(unsigned port, unsigned backlog) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("can't create backlog for acceptor socket");
    }
    const int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    sockaddr_in serv_addr{};
    socklen_t client_len = sizeof(sockaddr_in);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        throw std::runtime_error("can't bind acceptor socket");
    }
    if (listen(fd, backlog) < 0) {
        throw std::runtime_error("can't create backlog for acceptor socket");
    }

    return fd;
}

Server::~Server() {
    Server::shutdownAcceptor(_acceptorFd);

    //todo: destructor in std::shared_ptr
    std::for_each(_rings.begin(), _rings.end(), [](auto &ring){
        io_uring_queue_exit(ring.get());
    });
    io_uring_queue_exit(_mainRing.get());

    //todo: signal processing and thread termination
    std::for_each(_loopsThreads.begin(), _loopsThreads.end(), [](auto &thread) {
        thread->join();
    });
}

void Server::shutdownAcceptor(int fd) {
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

