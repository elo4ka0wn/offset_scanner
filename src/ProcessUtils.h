#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <sys/types.h>

struct MemoryRegion {
    uintptr_t start{};
    uintptr_t end{};
    std::string permissions;
    std::string pathname;

    size_t size() const { return static_cast<size_t>(end - start); }
    bool isReadable() const { return !permissions.empty() && permissions[0] == 'r'; }
    bool isWritable() const { return permissions.size() > 1 && permissions[1] == 'w'; }
};

struct ProcessInfo {
    pid_t pid{};
    std::string name;
};

namespace ProcessUtils {

std::vector<ProcessInfo> listProcesses();
std::optional<ProcessInfo> findProcessByName(const std::string &name);
std::vector<MemoryRegion> listMemoryRegions(pid_t pid);
std::vector<MemoryRegion> findModuleRegions(pid_t pid, const std::string &moduleName);
std::string describeMemoryRegion(const MemoryRegion &region);

}
