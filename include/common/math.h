#pragma once

#include <tuple>

namespace wg {

struct Extent2D : public std::pair<int, int> {
    Extent2D() = default;
    Extent2D(int width, int height) : std::pair<int, int>(width, height) {}
    int x() const { return first; }
    int y() const { return second; }
};

}