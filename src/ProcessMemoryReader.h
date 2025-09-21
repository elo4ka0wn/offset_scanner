#pragma once

#include <cstdint>
#include <optional>
#include <sys/types.h>

class ProcessMemoryReader {
public:
    explicit ProcessMemoryReader(pid_t pid);
    ~ProcessMemoryReader();

    ProcessMemoryReader(const ProcessMemoryReader &) = delete;
    ProcessMemoryReader &operator=(const ProcessMemoryReader &) = delete;

    ProcessMemoryReader(ProcessMemoryReader &&) noexcept;
    ProcessMemoryReader &operator=(ProcessMemoryReader &&) noexcept;

    bool isValid() const;
    bool read(uintptr_t address, void *buffer, size_t size) const;

    template <typename T>
    std::optional<T> readValue(uintptr_t address) const {
        T value{};
        if (!read(address, &value, sizeof(T))) {
            return std::nullopt;
        }
        return value;
    }

    pid_t pid() const { return pid_; }

private:
    pid_t pid_;
    int memFd_;
};
