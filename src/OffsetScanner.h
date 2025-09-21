#pragma once

#include "ProcessMemoryReader.h"
#include "ProcessUtils.h"
#include "Vector3.h"

#include <cstddef>
#include <optional>
#include <vector>

struct CandidateOffset {
    uintptr_t address{};
    ptrdiff_t offsetFromModule{};
    Vector3 value{};
};

class OffsetScanner {
public:
    OffsetScanner(pid_t pid, std::vector<MemoryRegion> moduleRegions, ProcessMemoryReader reader);

    std::vector<CandidateOffset> findCandidates(const Vector3 &target, float tolerance, size_t maxCandidates = 256) const;
    std::vector<CandidateOffset> verifyCandidates(const std::vector<CandidateOffset> &candidates, const Vector3 &expected, float tolerance) const;

    uintptr_t moduleBase() const { return moduleBase_; }
    const std::vector<MemoryRegion> &moduleRegions() const { return moduleRegions_; }

private:
    bool readVector(uintptr_t address, Vector3 &out) const;

    pid_t pid_{};
    std::vector<MemoryRegion> moduleRegions_;
    uintptr_t moduleBase_{};
    ProcessMemoryReader reader_;
};
