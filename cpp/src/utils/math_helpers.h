#pragma once
// Lightweight math types — mirrors Point and CellIndex type aliases

#include <cmath>
#include <cstddef>
#include <functional>
#include <optional>

namespace sag {

struct Point {
    float x = 0.0f;
    float y = 0.0f;

    bool operator==(const Point&) const = default;
};

struct CellIndex {
    int row = 0;
    int col = 0;

    bool operator==(const CellIndex&) const = default;
};

// Hash support for CellIndex as map key
struct CellIndexHash {
    std::size_t operator()(const CellIndex& c) const noexcept {
        auto h1 = std::hash<int>{}(c.row);
        auto h2 = std::hash<int>{}(c.col);
        return h1 ^ (h2 << 16);
    }
};

// ── Free functions ────────────────────────────────────────────────────

inline float distance(const Point& a, const Point& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

inline Point midpoint(const Point& a, const Point& b) {
    return {(a.x + b.x) / 2.0f, (a.y + b.y) / 2.0f};
}

inline std::optional<Point> resolve_feet_midpoint(
    const std::optional<Point>& left,
    const std::optional<Point>& right)
{
    if (left && right) return midpoint(*left, *right);
    if (left) return left;
    if (right) return right;
    return std::nullopt;
}

} // namespace sag
