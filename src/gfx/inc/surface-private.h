#pragma once

#include "platform/inc/platform.inc"

#include "gfx/surface.h"

#include "common/owned-resources.h"
#include "image-private.h"

#include <functional>

namespace wg {

struct WindowSurfaceResources {
    vk::raii::SurfaceKHR vk_surface{ nullptr };
};

struct SurfaceResources {
    vk::raii::SwapchainKHR vk_swapchain{ nullptr };
    vk::SurfaceFormatKHR vk_format;
    vk::Extent2D vk_extent;
    std::vector<vk::Image> vk_images;
    std::vector<vk::raii::ImageView> vk_image_views;
};

struct Surface::Impl {
    OwnedResourceHandle<WindowSurfaceResources> window_surface_resources;
    OwnedResourceHandle<SurfaceResources> resources;
    OwnedResourceHandle<ImageResources> color_image_resources;
    OwnedResourceHandle<GfxMemoryResources> color_memory_resources;
    OwnedResourceHandle<ImageResources> depth_image_resources;
    OwnedResourceHandle<GfxMemoryResources> depth_memory_resources;
    vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;

    static void SetFrameBufferSizeCallback(GLFWwindow* window, int width, int height);
    static std::map<GLFWwindow*, std::function<void(int, int)>> glfw_window_to_resized_func_map;
};

} // namespace wg