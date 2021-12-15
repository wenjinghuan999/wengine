#pragma once

#define VK_ENABLE_BETA_EXTENSIONS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

#include <functional>

namespace wg {

class Window;
struct Window::Impl {
    GLFWwindow* glfw_window{};
    std::function<void()> on_destroy;
};

} // namespace wg