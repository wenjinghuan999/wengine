#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <tuple>

namespace wg {

struct Extent2D : public std::pair<int, int> {
    Extent2D() = default;
    Extent2D(int width, int height) : std::pair<int, int>(width, height) {}
    [[nodiscard]] int x() const { return first; }
    [[nodiscard]] int y() const { return second; }
};

struct Transform {
    glm::mat4 transform;

    Transform() : transform(glm::identity<glm::mat4>()) {}
};

}