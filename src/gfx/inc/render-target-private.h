#pragma once

#include <functional>

#include "platform/inc/platform.inc"
#include "gfx/gfx.h"
#include "gfx/render-target.h"
#include "gfx/gfx-pipeline.h"
#include "common/owned-resources.h"
#include "gfx-constants-private.h"

namespace wg {

struct DrawCommandResources {
    vk::Pipeline pipeline;
    std::vector<vk::Buffer> vertex_buffers;
    std::vector<vk::DeviceSize> vertex_buffer_offsets;
    bool draw_indexed;
    vk::Buffer index_buffer;
    vk::DeviceSize index_buffer_offset;
    vk::IndexType index_type;
};

struct RenderTargetResources {
    vk::raii::RenderPass render_pass{nullptr};
    std::vector<vk::raii::Framebuffer> framebuffers;
    std::vector<vk::raii::Semaphore> image_available_semaphores;
    std::vector<vk::raii::Semaphore> render_finished_semaphores;
    std::vector<vk::raii::Fence> in_flight_fences;
    std::vector<vk::raii::PipelineLayout> pipeline_layout;
    std::vector<vk::raii::Pipeline> pipeline;
    std::vector<vk::CommandBuffer> command_buffers;

    vk::raii::Device* device{nullptr};
    std::vector<QueueInfo*> queues;
    QueueInfo* graphics_queue{nullptr};
    std::vector<DrawCommandResources> draw_command_resources;

    std::vector<vk::Fence> images_in_flight;
    int max_frames_in_flight{0};
    int current_frame_index{0};

    ~RenderTargetResources() {
        // handle manually for better performance
        device->waitIdle();
        (**device).freeCommandBuffers(*graphics_queue->vk_command_pool, command_buffers);
    }
};

struct RenderTarget::Impl {
    std::function<std::vector<vk::ImageView>()> get_image_views;
    OwnedResourceHandle<RenderTargetResources> resources;
};

}