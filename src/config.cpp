// project
#include "config.h"

// stl
#include <fstream>

// boost
#include <boost/lexical_cast.hpp>

const Config::ConfigParams &Config::params() const {
    return _params;
}

std::unordered_map<std::string, std::string> Config::stream2map(std::istream &istream) {
    std::string line;
    std::unordered_map<std::string, std::string> configMap;
    while (std::getline(istream, line)) {
        if (line.starts_with("#")) { continue; }
        std::istringstream iss(line);
        std::string key, value;
        iss >> key >> value;
        configMap.emplace(std::move(key), std::move(value));
    }
    return configMap;
}

const Config &Config::initFromFile(const std::string &path) {
    std::ifstream istream(path);
    if (!istream.is_open()) {
        throw std::runtime_error("config file not found, path : " + path);
    }
    auto configMap = Config::stream2map(istream);
    istream.close();

    ConfigParams params;
    params.port = configMap.contains("port") ? std::atoi(configMap.at("port").c_str()) : params.port;
    params.cpuLimit = configMap.contains("cpu_limit") ?
                      std::atoi(configMap.at("cpu_limit").c_str()) : params.cpuLimit;
    params.documentRoot = configMap.contains("document_root") ? configMap.at("document_root") : params.documentRoot;
    params.shareRingBackend = configMap.contains("share_ring_backend") ?
                              boost::lexical_cast<bool>(configMap.at("share_ring_backend")) : params.shareRingBackend;
    params.enableSqPoll = configMap.contains("enable_sq_poll") ?
                          boost::lexical_cast<bool>(configMap.at("enable_sq_poll")) : params.enableSqPoll;
    params.rlimitNoFile = configMap.contains("rlimit_nofile") ?
                          std::atoi(configMap.at("rlimit_nofile").c_str()) : params.rlimitNoFile;
    params.ringEntities = configMap.contains("ring_entities") ?
                          std::atoi(configMap.at("ring_entities").c_str()) : params.ringEntities;
    params.bufferSize = configMap.contains("buffer_size") ?
                        std::atoi(configMap.at("buffer_size").c_str()) : params.bufferSize;
    params.registerBuffers = configMap.contains("register_buffers") ?
                             boost::lexical_cast<bool>(configMap.at("register_buffers")) : params.registerBuffers;

    _params = params;

    return *this;
}
