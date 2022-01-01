#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <tuple>

namespace wg {

struct Extent2D : public std::pair<int, int> {
    Extent2D() = default;
    Extent2D(int width, int height) : std::pair<int, int>(width, height) {}
    [[nodiscard]] int x() const { return first; }
    [[nodiscard]] int y() const { return second; }
};

}