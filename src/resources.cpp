// project
#include "resources.h"
#include "config.h"

// stl
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>


void BufferManager::create(size_t chunkSize, size_t chunkCount) {
    _bufferSize = chunkSize;
    _buffersCount = chunkCount;

    _buffers.resize(_buffersCount * _bufferSize);
    std::memset(_buffers.data(), 0, _buffers.capacity());

    _buffersFilling.resize(_buffersCount);
    std::memset(_buffersFilling.data(), 0, _buffersFilling.capacity());
}

char *BufferManager::getBufferByFd(int fd) {
    return _buffers.data() + (fd * _bufferSize);
}

size_t BufferManager::bufferSize() {
    return get_const_instance()._bufferSize;
}

size_t BufferManager::buffersCount() {
    return get_const_instance()._buffersCount;
}

BufferManager::BufferManager() : _buffers(), _buffersFilling(), _bufferSize(), _buffersCount() {

}

void BufferManager::setBufferSize(size_t size, int fd) {
    _buffersFilling[fd] = size;
}

size_t BufferManager::getBufferSize(int fd) {
    return _buffersFilling[fd];
}

// Перехеширование не планируется. Создаем map с запасом. Load factor = 3 (https://habr.com/ru/post/250383/).
FileManager::FileManager() :
_map(Config::get_const_instance().params().rlimitNoFile, 3),
_basePath(Config::get_const_instance().params().documentRoot)
{
    if (!std::filesystem::exists(_basePath)) {
        throw std::runtime_error("document dir not exists");
    }
}

std::optional<FileManager::file_info_ref_it>  FileManager::getFileInfo(const std::string &filePath) {
    auto iter = _map.contains(filePath);
    if (iter != _map.end()) {
        return {std::cref(iter)};
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

    iter = _map.emplace(filePath, fd, statStruct.st_size);
    if (iter == _map.end()) {
        return {std::nullopt};
    }

    return {std::cref(iter)};
}

FileManager::FileInfo::FileInfo(size_t &&sizeRef, int &&fdRef)
: size(sizeRef), fd(fdRef) {

}

FileManager::FileInfo::~FileInfo() {
    if (close(fd)) {
        throw std::runtime_error("can't close fd");
    }
}
