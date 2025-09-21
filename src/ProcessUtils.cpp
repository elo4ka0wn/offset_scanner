#include "ProcessUtils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {

bool isNumber(const std::string &text) {
    return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });
}

std::string trim(const std::string &value) {
    const auto first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
}

} // namespace

namespace ProcessUtils {

std::vector<ProcessInfo> listProcesses() {
    std::vector<ProcessInfo> processes;
    const std::filesystem::path procPath{"/proc"};
    for (const auto &entry : std::filesystem::directory_iterator(procPath)) {
        if (!entry.is_directory()) {
            continue;
        }
        const std::string directoryName = entry.path().filename().string();
        if (!isNumber(directoryName)) {
            continue;
        }
        pid_t pid = static_cast<pid_t>(std::stol(directoryName));
        std::ifstream commFile(entry.path() / "comm");
        std::string processName;
        if (commFile) {
            std::getline(commFile, processName);
        }
        processes.push_back({pid, processName});
    }
    std::sort(processes.begin(), processes.end(), [](const ProcessInfo &lhs, const ProcessInfo &rhs) {
        return lhs.pid < rhs.pid;
    });
    return processes;
}

std::optional<ProcessInfo> findProcessByName(const std::string &name) {
    const auto processes = listProcesses();
    auto it = std::find_if(processes.begin(), processes.end(), [&](const ProcessInfo &info) {
        return info.name == name;
    });
    if (it == processes.end()) {
        return std::nullopt;
    }
    return *it;
}

std::vector<MemoryRegion> listMemoryRegions(pid_t pid) {
    std::vector<MemoryRegion> regions;
    std::ifstream mapsFile("/proc/" + std::to_string(pid) + "/maps");
    if (!mapsFile) {
        return regions;
    }
    std::string line;
    while (std::getline(mapsFile, line)) {
        std::istringstream iss(line);
        std::string addressRange;
        MemoryRegion region;
        std::string offset;
        std::string device;
        std::string inode;
        std::string pathname;
        if (!(iss >> addressRange >> region.permissions >> offset >> device >> inode)) {
            continue;
        }
        std::getline(iss, pathname);
        region.pathname = trim(pathname);
        const auto dashPos = addressRange.find('-');
        if (dashPos == std::string::npos) {
            continue;
        }
        region.start = std::stoull(addressRange.substr(0, dashPos), nullptr, 16);
        region.end = std::stoull(addressRange.substr(dashPos + 1), nullptr, 16);
        regions.push_back(region);
    }
    return regions;
}

std::vector<MemoryRegion> findModuleRegions(pid_t pid, const std::string &moduleName) {
    std::vector<MemoryRegion> matches;
    const auto regions = listMemoryRegions(pid);
    for (const auto &region : regions) {
        if (!region.pathname.empty()) {
            const auto namePos = region.pathname.find(moduleName);
            if (namePos != std::string::npos) {
                matches.push_back(region);
            }
        }
    }
    return matches;
}

std::string describeMemoryRegion(const MemoryRegion &region) {
    std::ostringstream oss;
    oss << "0x" << std::hex << region.start << "-0x" << region.end
        << " " << region.permissions;
    if (!region.pathname.empty()) {
        oss << " " << region.pathname;
    }
    return oss.str();
}

} // namespace ProcessUtils
