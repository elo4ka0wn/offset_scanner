#pragma once

#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

struct Vector3 {
    float x{};
    float y{};
    float z{};

    Vector3() = default;
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    std::array<float, 3> toArray() const {
        return {x, y, z};
    }

    std::string toString(int precision = 3) const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision)
            << "(" << x << ", " << y << ", " << z << ")";
        return oss.str();
    }

    static Vector3 parse(const std::string &text) {
        Vector3 result;
        char comma1 = 0;
        char comma2 = 0;
        std::istringstream iss(text);
        if (!(iss >> result.x >> comma1 >> result.y >> comma2 >> result.z) || comma1 != ',' || comma2 != ',') {
            throw std::invalid_argument("Vector3 must be in format x,y,z");
        }
        return result;
    }
};

inline bool approximatelyEqual(float lhs, float rhs, float tolerance) {
    return std::fabs(lhs - rhs) <= tolerance;
}

inline bool approximatelyEqual(const Vector3 &lhs, const Vector3 &rhs, float tolerance) {
    return approximatelyEqual(lhs.x, rhs.x, tolerance) &&
           approximatelyEqual(lhs.y, rhs.y, tolerance) &&
           approximatelyEqual(lhs.z, rhs.z, tolerance);
}

inline std::string formatOffsets(const std::vector<uintptr_t> &offsets) {
    std::ostringstream oss;
    for (size_t i = 0; i < offsets.size(); ++i) {
        oss << "0x" << std::hex << offsets[i];
        if (i + 1 != offsets.size()) {
            oss << " -> ";
        }
    }
    return oss.str();
}
