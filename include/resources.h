#ifndef IO_URING_STATIC_SERVER_RESOURCES_H
#define IO_URING_STATIC_SERVER_RESOURCES_H

// stl
#include <vector>

// boost
#include <boost/serialization/singleton.hpp>

/**
 * Manages buffers. The buffer is accessed by the connection descriptor.
 * It is worth allocating memory immediately for the number of connections.
 */
class BufferManager : public boost::serialization::singleton<BufferManager> {
public:
    void create(size_t chunkSize, size_t chunkCount);
    char *getBufferByFd(int fd);
    [[nodiscard]] static size_t bufferSize() ;
    [[nodiscard]] static size_t buffersCount() ;
    void setBufferSize(size_t size, int fd);
    size_t getBufferSize(int fd);
private:
    BufferManager();
    friend boost::serialization::detail::singleton_wrapper<BufferManager>;
    std::vector<char> _buffers;
    std::vector<size_t> _buffersFilling;
    size_t _bufferSize;
    size_t _buffersCount;
};

class FileManager : public boost::serialization::singleton<FileManager> {
private:
    FileManager();
    friend boost::serialization::detail::singleton_wrapper<FileManager>;
};

#endif //IO_URING_STATIC_SERVER_RESOURCES_H
