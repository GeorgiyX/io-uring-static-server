#ifndef IO_URING_STATIC_SERVER_UTILS_H
#define IO_URING_STATIC_SERVER_UTILS_H

// stl
#include <iostream>

namespace Utils {
    struct KernelVersion {
        unsigned kernel;
        unsigned major;
        unsigned minor;
        unsigned patch;

        bool operator<(const KernelVersion &version) const;
        explicit operator std::string() const;
        explicit operator unsigned int() const;
        static KernelVersion serverMinimal();
        static KernelVersion current();
    };

    bool isPrivileged();
}

std::ostream& operator<<(std::ostream& os, const Utils::KernelVersion& kernelVersion);

#endif //IO_URING_STATIC_SERVER_UTILS_H
