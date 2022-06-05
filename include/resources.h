#ifndef IO_URING_STATIC_SERVER_RESOURCES_H
#define IO_URING_STATIC_SERVER_RESOURCES_H

// boost
#include <boost/serialization/singleton.hpp>

class BufferManager : boost::serialization::singleton<BufferManager> {

};

class FileManager : boost::serialization::singleton<BufferManager> {

};

#endif //IO_URING_STATIC_SERVER_RESOURCES_H
