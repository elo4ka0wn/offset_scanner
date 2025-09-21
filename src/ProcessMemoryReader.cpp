#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "ProcessMemoryReader.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/uio.h>
#include <unistd.h>

namespace {

int openProcessMemory(pid_t pid) {
    const std::string path = "/proc/" + std::to_string(pid) + "/mem";
    const int fd = ::open(path.c_str(), O_RDONLY);
    return fd;
}

} // namespace

ProcessMemoryReader::ProcessMemoryReader(pid_t pid)
    : pid_(pid), memFd_(openProcessMemory(pid)) {}

ProcessMemoryReader::~ProcessMemoryReader() {
    if (memFd_ >= 0) {
        ::close(memFd_);
    }
}

ProcessMemoryReader::ProcessMemoryReader(ProcessMemoryReader &&other) noexcept
    : pid_(other.pid_), memFd_(other.memFd_) {
    other.memFd_ = -1;
}

ProcessMemoryReader &ProcessMemoryReader::operator=(ProcessMemoryReader &&other) noexcept {
    if (this != &other) {
        if (memFd_ >= 0) {
            ::close(memFd_);
        }
        pid_ = other.pid_;
        memFd_ = other.memFd_;
        other.memFd_ = -1;
    }
    return *this;
}

bool ProcessMemoryReader::isValid() const {
    return pid_ > 0;
}

bool ProcessMemoryReader::read(uintptr_t address, void *buffer, size_t size) const {
    if (size == 0) {
        return true;
    }
#if defined(__linux__)
    struct iovec local{};
    struct iovec remote{};
    local.iov_base = buffer;
    local.iov_len = size;
    remote.iov_base = reinterpret_cast<void *>(address);
    remote.iov_len = size;
    const ssize_t bytesRead = ::process_vm_readv(pid_, &local, 1, &remote, 1, 0);
    if (bytesRead == static_cast<ssize_t>(size)) {
        return true;
    }
#endif
    if (memFd_ >= 0) {
        const off_t offset = static_cast<off_t>(address);
        const ssize_t bytesRead = ::pread(memFd_, buffer, size, offset);
        if (bytesRead == static_cast<ssize_t>(size)) {
            return true;
        }
    }
    return false;
}
