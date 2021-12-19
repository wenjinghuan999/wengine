#include "gfx/gfx.h"
#include "gfx/command-buffer.h"
#include "common/logger.h"
#include "gfx-private.h"
#include "command-buffer-private.h"
#include "draw-command-private.h"

namespace {
    
[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

std::shared_ptr<CommandBuffer> CommandBuffer::Create(const std::shared_ptr<RenderTarget>& render_target) {
    return std::shared_ptr<CommandBuffer>(new CommandBuffer(render_target));
}

CommandBuffer::CommandBuffer(const std::shared_ptr<RenderTarget>& render_target)
    : render_target_(render_target), impl_(std::make_unique<Impl>()) {}

void Gfx::commitCommands(const std::shared_ptr<CommandBuffer>& command_buffer) {

    command_buffer->impl_->resources.reset();

    if (!logical_device_) {
        logger().error("Cannot create command buffer resources because logical device is not available.");
    }

    auto [width, height] = command_buffer->render_target()->extent();
    auto image_views = command_buffer->render_target()->impl_->get_image_views();
    auto render_target_resources = command_buffer->render_target()->impl_->resources->get();
    if (!render_target_resources) {
        logger().error("Cannot commit draw commands because render target resource has not been created.");
        return;
    }

    auto resources = std::make_unique<CommandBufferResources>();

    // Create command buffers
    auto& queue_info_array = logical_device_->impl_->queues[gfx_queues::graphics];
    if (!queue_info_array.empty()) {
        auto& queue_info = queue_info_array[0]; // Choose first for now
        resources->command_buffers.resize(image_views.size());
        auto command_buffer_allocate_info = vk::CommandBufferAllocateInfo{
            .commandPool = *queue_info->vk_command_pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = static_cast<uint32_t>(image_views.size())
        };
        // Do not use vk::raii::CommandBuffer because there might be compile errors on MSVC
        // Since we are allocating from the pool, we don't want to destruct the command buffer anyways.
        resources->command_buffers = 
            (*logical_device_->impl_->vk_device).allocateCommandBuffers(command_buffer_allocate_info);
    } else {
        logger().error("Cannot allocate command buffer because graphics queue has no command pool.");
    }

    // Record commands
    for (size_t i = 0; i < image_views.size(); i++)
    {
        auto& vk_command_buffer = resources->command_buffers[i];
        vk_command_buffer.begin({
            .flags            = {},
            .pInheritanceInfo = {},
        });

        auto clear_values = std::array{ vk::ClearValue{
            .color = { .float32 = std::array{ 0.f, 0.f, 0.f, 1.f } }
            } };
        auto render_pass_begin_info = vk::RenderPassBeginInfo{
            .renderPass  = *render_target_resources->render_pass,
            .framebuffer = *render_target_resources->framebuffers[i],
            .renderArea  = {
                .offset  = { 0, 0 },
                .extent  = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }
            }
        }   .setClearValues(clear_values);
        vk_command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        for (auto&& draw_command : command_buffer->draw_commands) {
            auto pipeline_resources = draw_command->pipeline()->impl_->resources->get();
            if (!pipeline_resources) {
                logger().warn("Pipeline resources are not available! Draw command is skipped.");
                continue;
            }

            vk_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_resources->pipeline);

            draw_command->impl_->draw(vk_command_buffer);
        }

        vk_command_buffer.endRenderPass();
        vk_command_buffer.end();
    }

    command_buffer->impl_->resources =
        logical_device_->impl_->command_buffer_resources.store(std::move(resources));
    command_buffer->ready_ = true;
}

} // namespace wg

