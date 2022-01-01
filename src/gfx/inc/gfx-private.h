#pragma once

#include "platform/inc/platform.inc"

#include <array>
#include <bitset>
#include <vector>
#include <tuple>

#include "gfx/gfx.h"
#include "common/owned-resources.h"
#include "gfx/inc/surface-private.h"
#include "gfx/inc/shader-private.h"
#include "gfx/inc/render-target-private.h"
#include "gfx/inc/gfx-buffer-private.h"

namespace wg {

struct WindowSurfaceResources {
    std::shared_ptr<Surface> surface;
};

struct Gfx::Impl {
    vk::raii::Context context;
    vk::raii::Instance instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT debug_messenger{nullptr};
    OwnedResources<WindowSurfaceResources> window_surfaces_;

    Gfx* gfx;
    void createBufferResources(const std::shared_ptr<GfxBufferBase>& gfx_buffer, vk::BufferUsageFlags usage);
};

struct PhysicalDevice::Impl {
    vk::raii::PhysicalDevice vk_physical_device{nullptr};
    // num_queues_total[queue_id] = total num of queues available in all families
    std::array<int, gfx_queues::NUM_QUEUES> num_queues_total{};
    // num_queues[queue_family_index] = num_queues_of_family
    std::vector<int> num_queues{};
    // queue_supports[queue_family_index] = bitset of support of each gfx_queue
    std::vector<std::bitset<gfx_queues::NUM_QUEUES>> queue_supports{};
    
    explicit Impl(vk::raii::PhysicalDevice vk_physical_device);
    int findMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags property_flags);
    bool allocateQueues(
        std::array<int, gfx_queues::NUM_QUEUES> required_queues,
        const std::vector<vk::SurfaceKHR>& surfaces,
        std::array<std::vector<std::pair<uint32_t, uint32_t>>, gfx_queues::NUM_QUEUES>& out_queue_index_remap);
    bool tryAllocateQueues(
        std::array<int, gfx_queues::NUM_QUEUES> required_queues,
        const std::vector<std::bitset<gfx_queues::NUM_QUEUES>>& real_queue_supports,
        std::vector<std::map<gfx_queues::QueueId, int>>& out_allocated_queue_counts);
    bool checkQueueSupport(
        uint32_t queue_family_index, gfx_queues::QueueId queue_id,
        const std::vector<vk::SurfaceKHR>& surfaces);
protected:
    Impl() = default;
};

struct LogicalDevice::Impl {
    vk::raii::Device vk_device;
    // allocated_queues[queue_family_index][] = QueueInfo
    std::vector<std::vector<std::unique_ptr<QueueInfo>>> allocated_queues;
    // queue_references[queue_id][] = QueueInfoRef
    std::array<std::vector<QueueInfoRef>, gfx_queues::NUM_QUEUES> queue_references;

    // resources of surfaces (may be accessed by surface using OwnedResourcesHandle)
    OwnedResources<SurfaceResources> surface_resources;
    // resources of shaders (may be accessed by shader using OwnedResourcesHandle)
    OwnedResources<ShaderResources> shader_resources;
    // resources of render targets (may be accessed by render target using OwnedResourcesHandle)
    OwnedResources<RenderTargetResources> render_target_resources;
    // resources of buffers (may be accessed by buffer using OwnedResourcesHandle)
    OwnedResources<BufferResources> buffer_resources;
    
    explicit Impl(vk::raii::Device vk_device) 
        : vk_device(std::move(vk_device)) {}
};

}