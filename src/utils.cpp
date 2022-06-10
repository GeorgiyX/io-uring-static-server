// project
#include "utils.h"

// stl
#include <unistd.h>
#include <sys/utsname.h>
#include <stdexcept>
#include <sys/resource.h>
#include <string.h>

bool Utils::isPrivileged() {
    return !geteuid();
}

void Utils::increaseResourceLimit(int flag, rlim_t rlim) {
    rlimit limits{};
    if (getrlimit(flag, &limits)) {
        throw std::runtime_error("getrlimit not successful (flag: " + std::to_string(flag) + ")");
    }
    limits.rlim_cur = rlim;

    if (setrlimit(flag, &limits)) {
        throw std::runtime_error(std::string("setrlimit not successful (flag: ") +
        std::to_string(flag) + ", val:" +  std::to_string(limits.rlim_cur) + "): " + strerror(errno));
    }
}

void move2digit(char **string) {
    auto ptr = *string;
    while (*ptr) {
        if (std::isdigit(*ptr)) {
            *string = ptr;
            return;
        }
        ptr++;
    }
    *string = ptr;
}

bool Utils::KernelVersion::operator<(const Utils::KernelVersion &version) const {
    return this->operator unsigned int() < version.operator unsigned int();
}

Utils::KernelVersion Utils::KernelVersion::current() {
    utsname unameData{};
    if (uname(&unameData)) {
        throw std::runtime_error("error in uname");
    }

    KernelVersion version{};
    auto releasePtr = unameData.release;
    version.kernel = std::strtol(releasePtr, &releasePtr, 10);
    move2digit(&releasePtr);
    version.major = std::strtol(releasePtr, &releasePtr, 10);
    move2digit(&releasePtr);
    version.minor = std::strtol(releasePtr, &releasePtr, 10);
    move2digit(&releasePtr);
    version.patch = std::strtol(releasePtr, &releasePtr, 10);

    return version;
}

std::ostream &operator<<(std::ostream &os, const Utils::KernelVersion &kernelVersion) {
    return os << std::string(kernelVersion);
}

Utils::KernelVersion Utils::KernelVersion::serverMinimal() {
    KernelVersion kernelVersion{};
    kernelVersion.kernel = 5;
    kernelVersion.major = 11;
    kernelVersion.minor = 0;
    kernelVersion.patch = 0;
    return kernelVersion;
}

Utils::KernelVersion::operator std::string() const {
    return std::to_string(kernel) + "." + std::to_string(major) +
    "." + std::to_string(minor) + "." + std::to_string(patch);
}

Utils::KernelVersion::operator unsigned int() const {
    return kernel * 1000000 + major * 10000 + minor * 1000 + patch;
}
