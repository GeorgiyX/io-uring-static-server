#ifndef IO_URING_STATIC_SERVER_RESOURCES_H
#define IO_URING_STATIC_SERVER_RESOURCES_H

// stl
#include <vector>
#include <filesystem>

// boost
#include <boost/serialization/singleton.hpp>

// cds
#include <cds/container/michael_map_nogc.h>
#include <cds/container/michael_kvlist_nogc.h>

/**
 * Manages buffers. The buffer is accessed by the connection descriptor.
 * It is worth allocating memory immediately for the number of connections.
 */
class BufferManager : public boost::serialization::singleton<BufferManager> {
public:
    void create(size_t bufferSize, size_t bufferCount);
    char *getBuffer(int fd);
    [[nodiscard]] static size_t fullBufferSize() ;
    [[nodiscard]] static size_t buffersCount() ;
    size_t bufferSize();
    void setBufferFilling(size_t size, int fd);
    size_t getBufferFilling(int fd);

private:
    BufferManager();
    friend boost::serialization::detail::singleton_wrapper<BufferManager>;
    std::vector<char> _buffers;
    std::vector<size_t> _buffersFilling;
    size_t _bufferSize;
    size_t _buffersCount;
};

/** It is important to follow the order of initialization of singletons */
class FileManager : public boost::serialization::singleton<FileManager> {
public:
    struct FileInfo {
        FileInfo() = default;
        FileInfo(size_t &&sizeRef, int &&fdRef);
        ~FileInfo();
        size_t size;
        int fd;
    };

    using cds_list = cds::container::MichaelKVList<cds::gc::nogc, std::string, FileInfo>;
    using cds_map = cds::container::MichaelHashMap<cds::gc::nogc, cds_list>;
    using file_info_ref_it = std::reference_wrapper<const cds_map::iterator>;

    std::optional<file_info_ref_it> getFileInfo(const std::string &filePath);
private:
    FileManager();
    friend boost::serialization::detail::singleton_wrapper<FileManager>;
    cds_map _map;
    std::filesystem::path _basePath;
};

#endif //IO_URING_STATIC_SERVER_RESOURCES_H
