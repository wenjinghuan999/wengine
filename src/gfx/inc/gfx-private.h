#pragma once

#include "platform/inc/platform.inc"

#include <array>
#include <vector>
#include <tuple>

#include "gfx/gfx.h"
#include "gfx/inc/shader-private.h"

namespace {

inline vk::QueueFlags GetRequiredQueueFlags(wg::gfx_queues::QueueId queue_id) {
    switch (queue_id)
    {
    case wg::gfx_queues::graphics:
    case wg::gfx_queues::present:
        return vk::QueueFlagBits::eGraphics;
    case wg::gfx_queues::transfer:
        return vk::QueueFlagBits::eTransfer;
    case wg::gfx_queues::compute:
        return vk::QueueFlagBits::eCompute;
    default:
        return {};
    }
}

}

namespace wg {

struct Gfx::Impl {
    vk::raii::Context context;
    vk::raii::Instance instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT debug_messenger{nullptr};
};

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

struct PhysicalDevice::Impl {
    vk::raii::PhysicalDevice vk_physical_device;
    // num_queues_total[queue_id] = total num of queues available in all families
    std::array<int, gfx_queues::NUM_QUEUES> num_queues_total{};
    // num_queues[queue_id] = <queue_family_index, num_queues_of_family>[]
    std::array<std::vector<std::pair<uint32_t, uint32_t>>, gfx_queues::NUM_QUEUES> num_queues{};
    
    explicit Impl(vk::raii::PhysicalDevice vk_physical_device) 
        : vk_physical_device(std::move(vk_physical_device)) {
        auto queue_families = this->vk_physical_device.getQueueFamilyProperties();
        for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
            auto queue_id = static_cast<gfx_queues::QueueId>(i);
            vk::QueueFlags required_flags = GetRequiredQueueFlags(queue_id);
            for (uint32_t queue_family_index = 0; 
                queue_family_index < static_cast<uint32_t>(queue_families.size()); 
                ++queue_family_index) {
                
                const auto& queue_family = queue_families[queue_family_index];
                if (queue_family.queueFlags & required_flags) {
                    num_queues[i].emplace_back(queue_family_index, queue_family.queueCount);
                    num_queues_total[i] += static_cast<int>(queue_family.queueCount);
                }
            }
        }
    }
};

struct QueueInfo {
    uint32_t queue_family_index{};
    uint32_t queue_index_in_family{};
    vk::raii::Queue vk_queue{nullptr};
};

struct LogicalDevice::Impl {
    vk::raii::Device vk_device;
    // queues[queue_id][queue_index] = <queue_family_index, vk_queue>
    std::map<gfx_queues::QueueId, std::vector<std::unique_ptr<QueueInfo>>> queues;
    // resources of surfaces (may be accessed by surface using OwnedResourcesHandle)
    OwnedResources<SurfaceResources> surface_resources;
    // resources of shaders (may be accessed by shader using OwnedResourcesHandle)
    OwnedResources<ShaderResources> shader_resources;
    
    explicit Impl(vk::raii::Device vk_device) 
        : vk_device(std::move(vk_device)) {}
};

}