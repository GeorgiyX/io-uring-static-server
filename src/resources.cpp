// project
#include "resources.h"
#include "config.h"
#include "utils.h"
#include "logger.h"

// stl
#include <cstring>
#include <utility>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/resource.h>


void BufferManager::create(size_t bufferSize, size_t bufferCount) {
    _bufferSize = bufferSize;
    _buffersCount = bufferCount;

    _buffers.resize(_buffersCount * _bufferSize);
    std::memset(_buffers.data(), 0, _buffers.capacity());

    _buffersFilling.resize(_buffersCount);
    std::memset(_buffersFilling.data(), 0, _buffersFilling.capacity());
}

char *BufferManager::getBuffer(int fd) {
    return _buffers.data() + (fd * _bufferSize);
}

size_t BufferManager::fullBufferSize() {
    return get_const_instance()._buffers.size();
}

size_t BufferManager::buffersCount() {
    return get_const_instance()._buffersCount;
}

BufferManager::BufferManager() : _buffers(), _buffersFilling(), _bufferSize(), _buffersCount() {

}

void BufferManager::setBufferFilling(size_t size, int fd) {
    _buffersFilling[fd] = size;
}

size_t BufferManager::getBufferFilling(int fd) {
    return _buffersFilling[fd];
}

size_t BufferManager::bufferSize() const {
    return _bufferSize - 1 /* zero terminate */;
}

// No hashing is planned. Creating a map with reserve. Load factor = 3 (https://habr.com/ru/post/250383/).
FileManager::FileManager() :
_map(Config::get_const_instance().params().rlimitNoFile, 3),
_basePath(Config::get_const_instance().params().documentRoot)
{
    if (!std::filesystem::exists(_basePath)) {
        throw std::runtime_error("document dir not exists");
    }

    Utils::increaseResourceLimit(RLIMIT_NOFILE, Config::get_const_instance().params().rlimitNoFile);
}

std::optional<FileManager::file_info_ref> FileManager::getFileInfo(const std::string &filePath) {
    auto iter = _map.contains(filePath);
    if (iter != _map.end()) {
        return {std::cref(iter->second)};
    }
    auto path = _basePath / filePath;
    if (!path.has_filename()) {
        path.replace_filename("index.html");
    }

    if (!std::filesystem::exists(path)) {
        return {std::nullopt};
    }

    auto fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        throw std::runtime_error("can't open file: " + path.string());
    }
    struct stat statStruct{};
    if(fstat(fd, &statStruct) < 0)
    {
        close(fd);
        throw std::runtime_error("can't fstat file");
    }

    iter = _map.emplace(filePath, fd, statStruct.st_size, path.extension().string());
    if (iter == _map.end()) {
        return {std::nullopt};
    }

    return {std::cref(iter->second)};
}

FileManager::FileInfo::~FileInfo() {
    close(fd);
}

FileManager::FileInfo::FileInfo(int _fd, size_t _size, std::string _extension) :
fd(_fd), size(_size), extension(std::move(_extension)) {
}

FileManager::FileInfo::FileInfo(FileInfo &&fileInfo) :
fd(fileInfo.fd), size(fileInfo.size), extension(std::move(fileInfo.extension)){
    fileInfo.fd = -1;
}
