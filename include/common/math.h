#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <tuple>

namespace wg {

struct Size2D : public std::pair<int, int> {
    Size2D() = default;
    Size2D(int width, int height) : std::pair<int, int>(width, height) {}
    [[nodiscard]] int x() const { return first; }
    [[nodiscard]] int y() const { return second; }
};

struct Transform {
    glm::mat4 transform{ glm::identity<glm::mat4>() };
};

} // namespace wg