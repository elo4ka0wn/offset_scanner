#include "OffsetScanner.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <unordered_set>

namespace {
constexpr size_t vectorSizeBytes = sizeof(float) * 3;
constexpr size_t scanChunkSize = 64 * 1024;
constexpr size_t chunkOverlap = vectorSizeBytes - sizeof(float);
}

OffsetScanner::OffsetScanner(pid_t pid, std::vector<MemoryRegion> moduleRegions, ProcessMemoryReader reader)
    : pid_(pid), moduleRegions_(std::move(moduleRegions)), reader_(std::move(reader)) {
    moduleBase_ = std::numeric_limits<uintptr_t>::max();
    for (const auto &region : moduleRegions_) {
        moduleBase_ = std::min(moduleBase_, region.start);
    }
    if (moduleBase_ == std::numeric_limits<uintptr_t>::max()) {
        moduleBase_ = 0;
    }
}

std::vector<CandidateOffset> OffsetScanner::findCandidates(const Vector3 &target, float tolerance, size_t maxCandidates) const {
    std::vector<CandidateOffset> candidates;
    std::vector<uint8_t> buffer;
    std::unordered_set<uintptr_t> seenAddresses;
    for (const auto &region : moduleRegions_) {
        if (!region.isReadable()) {
            continue;
        }
        uintptr_t current = region.start;
        while (current < region.end) {
            const size_t bytesRemaining = static_cast<size_t>(region.end - current);
            const size_t bytesToRead = std::min(scanChunkSize, bytesRemaining);
            buffer.resize(bytesToRead);
            if (!reader_.read(current, buffer.data(), bytesToRead)) {
                current += bytesToRead;
                continue;
            }
            if (bytesToRead < vectorSizeBytes) {
                break;
            }
            const size_t lastStart = bytesToRead - vectorSizeBytes;
            for (size_t offset = 0; offset <= lastStart; offset += sizeof(float)) {
                Vector3 vector{};
                std::memcpy(&vector.x, buffer.data() + offset, sizeof(float));
                std::memcpy(&vector.y, buffer.data() + offset + sizeof(float), sizeof(float));
                std::memcpy(&vector.z, buffer.data() + offset + 2 * sizeof(float), sizeof(float));
                if (!approximatelyEqual(vector, target, tolerance)) {
                    continue;
                }
                const uintptr_t absoluteAddress = current + offset;
                if (seenAddresses.insert(absoluteAddress).second) {
                    candidates.push_back({absoluteAddress, static_cast<ptrdiff_t>(absoluteAddress - moduleBase_), vector});
                    if (candidates.size() >= maxCandidates) {
                        return candidates;
                    }
                }
            }
            if (bytesToRead == bytesRemaining) {
                break;
            }
            uintptr_t next = current + bytesToRead;
            if (chunkOverlap < bytesToRead) {
                next -= chunkOverlap;
            }
            if (next <= current) {
                next = current + bytesToRead;
            }
            current = next;
        }
    }
    return candidates;
}

std::vector<CandidateOffset> OffsetScanner::verifyCandidates(const std::vector<CandidateOffset> &candidates, const Vector3 &expected, float tolerance) const {
    std::vector<CandidateOffset> filtered;
    filtered.reserve(candidates.size());
    for (const auto &candidate : candidates) {
        Vector3 currentVector;
        if (!readVector(candidate.address, currentVector)) {
            continue;
        }
        if (approximatelyEqual(currentVector, expected, tolerance)) {
            CandidateOffset updated = candidate;
            updated.value = currentVector;
            filtered.push_back(updated);
        }
    }
    return filtered;
}

bool OffsetScanner::readVector(uintptr_t address, Vector3 &out) const {
    std::array<float, 3> values{};
    if (!reader_.read(address, values.data(), sizeof(values))) {
        return false;
    }
    out = Vector3{values[0], values[1], values[2]};
    return true;
}
