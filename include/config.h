#ifndef IO_URING_STATIC_SERVER_CONFIG_H
#define IO_URING_STATIC_SERVER_CONFIG_H

// stl
#include <string>
#include <iostream>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <thread>

// boost
#include <boost/serialization/singleton.hpp>

struct Config : boost::serialization::singleton<Config> {
    struct ConfigParams {
        unsigned port = 5000;
        unsigned cpuLimit = std::thread::hardware_concurrency();
        std::string documentRoot = "./";
        bool shareRingBackend = true;
        bool enableSqPoll = true;
        unsigned rlimitNoFile = 8192;
        unsigned ringEntities = 8192;
        unsigned bufferSize = 4096;
        bool registerBuffers = true;
    };

    const Config &initFromFile(const std::string &path);
    [[nodiscard]] const ConfigParams &params() const;

private:
    Config() = default;
    friend boost::serialization::detail::singleton_wrapper<Config>;
    static std::unordered_map<std::string, std::string> stream2map(std::istream &istream);
    ConfigParams _params;
};

#endif //IO_URING_STATIC_SERVER_CONFIG_H
