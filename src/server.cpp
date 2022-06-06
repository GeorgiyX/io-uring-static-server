// project
#include "server.h"
#include "config.h"
#include "resources.h"
#include "utils.h"

// stl
#include <cstring>
#include <algorithm>
#include <sys/resource.h>
#include <cstdint>

Server::Server(const std::string &configPath) : _rings(), _mainRing() {
    const auto &params = Config::get_mutable_instance().initFromFile(configPath).params();

    std::cout << "creating io_rings..." << std::endl;
    _mainRing = Server::createRing();
    for (size_t index = 0; index < params.cpuLimit; index++) {
        _rings.push_back(Server::createRing(_mainRing->ring_fd));
    }

    std::cout << "allocating buffers..." << std::endl;
    BufferManager::get_mutable_instance().create(params.bufferSize, params.ringEntities);
    if (params.registerBuffers) {
        registerBuffers();
    }
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
        iovectors[index].iov_len = BufferManager::bufferSize();
    }

    rlimit rlimitMemlock{};
    if (!getrlimit(RLIMIT_MEMLOCK, &rlimitMemlock)) {
        throw std::runtime_error("getrlimit RLIMIT_MEMLOCK not successful");
    }
    rlimitMemlock.rlim_cur = rlimitMemlock.rlim_cur + BufferManager::bufferSize();

    if (rlimitMemlock.rlim_max < rlimitMemlock.rlim_cur) {
        if (!Utils::isPrivileged()) {
            throw std::runtime_error("it is not possible to increase RLIMIT_MEMLOCK, super user rights are required");
        }
        rlimitMemlock.rlim_max = rlimitMemlock.rlim_max + BufferManager::bufferSize();
    }

    if (setrlimit(RLIMIT_MEMLOCK, &rlimitMemlock)) {
        throw std::runtime_error("setrlimit RLIMIT_MEMLOCK not successful");
    }

    if (io_uring_register_buffers(_mainRing.get(), iovectors.data(), iovectors.size())) {
        throw std::runtime_error("can't register buffers");
    }
}

