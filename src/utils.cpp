// project
#include "utils.h"

// stl
#include <unistd.h>
#include <sys/utsname.h>
#include <stdexcept>

#include <liburing.h>
#include <cassert>
#include <unistd.h>
#include <cstring>


void hello_world() {
    io_uring_params params{};
    io_uring ring{};
    memset(&params, 0, sizeof(params));

    /**
     * Создаем инстанс io_uring, не используем никаких кастомных опций.
     * Емкость SQ и CQ буфера указываем как 4096 вхождений.
     */
    int ret = io_uring_queue_init_params(4, &ring, &params);
    assert(ret == 0);

    char hello[] = "hello world!\n";

    // Добавляем операцию write в очередь SQ.
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_write(sqe, STDOUT_FILENO, hello, 13, 0);

    // Сообщаем io_uring о новых SQE в SQ.
    io_uring_submit(&ring);

    // Ждем пока в CQ появится новое CQE.
    struct io_uring_cqe *cqe;
    ret = io_uring_wait_cqe(&ring, &cqe);
    assert(ret == 0);

    // Проверяем отсутствие ошибок.
    assert(cqe->res > 0);

    // Dequeue из очереди CQ.
    io_uring_cqe_seen(&ring, cqe);

    io_uring_queue_exit(&ring);
}

bool Utils::isPrivileged() {
    return !geteuid();
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
