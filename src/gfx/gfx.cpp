#include "gfx/gfx.h"

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <spdlog/spdlog.h>

namespace wg {

void Gfx::logExtensions() {
    uint32_t extensionCount = 0;
    vk::Result result = vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    if (result != vk::Result::eSuccess) {
        spdlog::error("Failed to get extensions, code {}.", result);
    }

    spdlog::info("{} extensions supported.", extensionCount);
}

}