#pragma once

#include "platform/inc/platform.inc"

#include <array>
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

    int findMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags property_flags) {
        auto memory_properties = vk_physical_device.getMemoryProperties();
        for (int i = 0; i < static_cast<int>(memory_properties.memoryTypeCount); i++) {
            if (requirements.memoryTypeBits & (1 << i) && 
                (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
                return i;
            }
        }
        return -1;
    }
};

struct LogicalDevice::Impl {
    vk::raii::Device vk_device;
    // queues[queue_id][queue_index] = <queue_family_index, vk_queue, vk_command_poll>
    std::map<gfx_queues::QueueId, std::vector<std::unique_ptr<QueueInfo>>> queues;

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