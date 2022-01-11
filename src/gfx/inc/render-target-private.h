#pragma once

#include <algorithm>
#include <iterator>
#include <functional>

#include "platform/inc/platform.inc"
#include "gfx/gfx.h"
#include "gfx/render-target.h"
#include "gfx/gfx-pipeline.h"
#include "common/owned-resources.h"
#include "gfx-constants-private.h"

namespace wg {

struct RenderTargetDrawCommandResources {
    vk::Pipeline pipeline;
    vk::PipelineLayout pipeline_layout;
    vk::DescriptorSet descriptor_set;
};

struct RenderTargetPipelineResources {
    vk::raii::Pipeline pipeline{nullptr};
    vk::raii::DescriptorPool descriptor_pool{nullptr};
    std::vector<vk::raii::DescriptorSet> descriptor_sets;
};

struct RenderTargetResources {
    vk::raii::RenderPass render_pass{nullptr};
    std::vector<vk::raii::Framebuffer> framebuffers;
    std::vector<vk::raii::Semaphore> image_available_semaphores;
    std::vector<vk::raii::Semaphore> render_finished_semaphores;
    std::vector<vk::raii::Fence> in_flight_fences;
    std::vector<vk::CommandBuffer> command_buffers;
    std::vector<RenderTargetPipelineResources> pipeline_resources;

    vk::raii::Device* device{nullptr};
    std::vector<QueueInfoRef> queues; // queues needed for render target
    int graphics_queue_index{-1};
    // draw_command_resources[...][image_index]
    std::vector<std::vector<RenderTargetDrawCommandResources>> draw_command_resources;

    std::vector<vk::Fence> images_in_flight;
    int max_frames_in_flight{0};
    int current_frame_index{0};

    ~RenderTargetResources() {
        // handle manually for better performance
        if (graphics_queue_index >= 0) {
            device->waitIdle();
            auto vk_command_pool = queues[graphics_queue_index].vk_command_pool;
            (**device).freeCommandBuffers(vk_command_pool, command_buffers);
        }
    }
};

struct RenderTarget::Impl {
    std::function<std::vector<vk::ImageView>()> get_image_views;
    OwnedResourceHandle<RenderTargetResources> resources;
};

}