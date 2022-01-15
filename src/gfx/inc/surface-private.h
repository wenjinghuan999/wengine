#pragma once

#include <functional>

#include "platform/inc/platform.inc"
#include "gfx/surface.h"
#include "common/owned-resources.h"

namespace wg {

struct SurfaceResources {
    vk::raii::SwapchainKHR vk_swapchain{ nullptr };
    vk::SurfaceFormatKHR vk_format;
    vk::Extent2D vk_extent;
    std::vector<vk::Image> vk_images;
    std::vector<vk::raii::ImageView> vk_image_views;
};

struct Surface::Impl {
    vk::raii::SurfaceKHR vk_surface{ nullptr };
    OwnedResourceHandle <SurfaceResources> resources;

    static void SetFrameBufferSizeCallback(GLFWwindow* window, int width, int height);
    static std::map<GLFWwindow*, std::function<void(int, int)>> glfw_window_to_resized_func_map;
};

} // namespace wg