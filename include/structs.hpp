#pragma once

#include <array>
#include <cmath>
#include <cstddef>

// Point in 2.5D/3D space: [0] = x, [1] = y, [2] = z (layer / elevation)
using Point = std::array<float, 3>;

constexpr size_t DEFAULT_MAX_LEAF_SIZE = 10;

[[nodiscard]]
inline float distanceSquared(Point const& a, Point const& b) {
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    float dz = a[2] - b[2];
    return (dx * dx) + (dy * dy) + (dz * dz);
}

[[nodiscard]]
inline float distanceSquared2D(Point const& a, Point const& b) {
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    return (dx * dx) + (dy * dy);
}