#include "gfx/gfx.h"
#include "gfx/renderer.h"
#include "common/logger.h"
#include "gfx-private.h"
#include "draw-command-private.h"

namespace {
    
[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

void Gfx::submitDrawCommands(const std::shared_ptr<RenderTarget>& render_target) {

    if (!logical_device_) {
        logger().error("Cannot create submit draw commands because logical device is not available.");
        return;
    }
    waitDeviceIdle();

    auto [width, height] = render_target->extent();
    auto image_views = render_target->impl_->get_image_views();
    auto* resources = render_target->impl_->resources.get();
    if (!resources) {
        logger().error("Cannot submit draw commands because render target resource has not been created.");
        return;
    }
    if (resources->command_buffers.size() != resources->framebuffers.size() ||
        resources->command_buffers.size() != image_views.size()) {
        logger().error("Cannot submit draw commands because command buffer count does not match framebuffer/image view count.");
        return;
    }

    auto draw_commands = render_target->renderer()->getDrawCommands();
    for (auto&& draw_command : draw_commands) {
        createPipelineResources(render_target, draw_command->pipeline());
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
            .renderPass  = *resources->render_pass,
            .framebuffer = *resources->framebuffers[i],
            .renderArea  = {
                .offset  = { 0, 0 },
                .extent  = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }
            }
        }   .setClearValues(clear_values);
        vk_command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        for (size_t i = 0; i < draw_commands.size(); ++i) {
            const auto& draw_command = draw_commands[i];
            const auto& draw_command_resources = resources->draw_command_resources[i];

            vk_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, draw_command_resources.pipeline);
            vk_command_buffer.bindVertexBuffers(0, draw_command_resources.vertex_buffers, draw_command_resources.vertex_buffer_offsets);

            if (draw_command_resources.draw_indexed) {
                vk_command_buffer.bindIndexBuffer(
                    draw_command_resources.index_buffer, draw_command_resources.index_buffer_offset, draw_command_resources.index_type);
            }

            draw_command->impl_->draw(vk_command_buffer);
        }

        vk_command_buffer.endRenderPass();
        vk_command_buffer.end();
    }
}

} // namespace wg

