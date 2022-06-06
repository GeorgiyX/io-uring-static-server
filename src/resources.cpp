// project
#include "resources.h"

// stl
#include <cstring>

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
