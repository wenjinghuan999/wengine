#pragma once

#include "platform/inc/platform.inc"
#include "gfx/surface.h"
#include "common/owned_resources.h"

namespace wg {

struct SurfaceResources {
    vk::raii::SwapchainKHR vk_swapchain{nullptr};
    vk::SurfaceFormatKHR vk_format;
    vk::Extent2D vk_extent;
    std::vector<VkImage> vk_images;
    std::vector<vk::raii::ImageView> vk_image_views;
};

struct Surface::Impl {
    vk::raii::SurfaceKHR vk_surface;
    OwnedResourceHandle resources_handle;
    Impl() : vk_surface(nullptr) {}
};

}