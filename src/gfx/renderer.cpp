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

Renderer& Renderer::addUniformBuffer(const std::shared_ptr<UniformBufferBase>& uniform_buffer) {
    uniform_buffers_[uniform_buffer->description().attribute] = uniform_buffer;
    return *this;
}

void Renderer::clearUniformBuffers() {
    uniform_buffers_.clear();
}

bool Renderer::valid() const {
    for (auto&& draw_command : getDrawCommands()) {
        if (!draw_command->pipeline()) {
            return false;
        }
        for (auto&& description : draw_command->pipeline()->uniform_layout().descriptions()) {
            if (description.attribute >= uniform_attributes::FRAMEBUFFER_UNIFORMS_START &&
                description.attribute < uniform_attributes::FRAMEBUFFER_UNIFORMS_END) {
                if (uniform_buffers_.find(description.attribute) == uniform_buffers_.end()) {
                    return false;
                }
            }
        }
    }
    return true;
}

void Gfx::submitDrawCommands(const std::shared_ptr<RenderTarget>& render_target) {

    if (!logical_device_) {
        logger().error("Cannot create submit draw commands because logical device is not available.");
        return;
    }
    waitDeviceIdle();

    auto [width, height] = render_target->extent();
    auto image_views = render_target->impl_->get_image_views();
    auto image_count = image_views.size();
    auto* resources = render_target->impl_->resources.get();
    if (!resources) {
        logger().error("Cannot submit draw commands because render target resource has not been created.");
        return;
    }
    if (resources->command_buffers.size() != resources->framebuffer_resources.size() ||
        resources->command_buffers.size() != image_count) {
        logger().error("Cannot submit draw commands because command buffer count does not match framebuffer/image view count.");
        return;
    }

    auto draw_commands_ = render_target->renderer()->getDrawCommands();
    for (auto&& draw_command : draw_commands_) {
        createDrawCommandResourcesForRenderTarget(render_target, draw_command);
    }

    // Record commands
    for (size_t i = 0; i < image_count; i++) {
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
            .framebuffer = *resources->framebuffer_resources[i].framebuffer,
            .renderArea  = {
                .offset  = { 0, 0 },
                .extent  = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }
            }
        }   .setClearValues(clear_values);
        vk_command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        for (size_t j = 0; j < draw_commands_.size(); ++j) {
            const auto& draw_command = draw_commands_[j];
            const auto& draw_command_resources = resources->draw_command_resources[j][i];
    
            vk_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, 
                draw_command_resources.pipeline_layout, 0, { draw_command_resources.descriptor_set }, {});
            vk_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, draw_command_resources.pipeline);
            draw_command->getImpl()->draw(vk_command_buffer);
        }

        vk_command_buffer.endRenderPass();
        vk_command_buffer.end();
    }
}

void Gfx::commitFramebufferUniformBuffers(
    const std::shared_ptr<RenderTarget>& render_target,
    uniform_attributes::UniformAttribute specified_attribute) {

    auto& renderer = render_target->renderer_;
    if (!renderer) {
        logger().error("Cannot commit framebuffer uniform buffer because renderer is not available.");
        return;
    }

    if (!logical_device_) {
        logger().error("Cannot commit framebuffer uniform buffer because logical device is not available.");
        return;
    }

    auto* resources = render_target->impl_->resources.get();
    if (!resources) {
        logger().error("Cannot commit framebuffer uniform buffer because render target resources are not available.");
        return;
    }

    size_t image_count = resources->command_buffers.size();
    for (size_t i = 0; i < image_count; ++i) {
        auto& framebuffer_resources = resources->framebuffer_resources[i];
        for (auto&& [attribute, cpu_uniform] : renderer->uniform_buffers_) {
            if (specified_attribute == uniform_attributes::none || specified_attribute == attribute) {
                // Find GPU uniform data
                const auto* gpu_uniform = [attribute, &framebuffer_resources]() 
                    -> const std::shared_ptr<UniformBufferBase>* {
                    for (auto&& uniform_buffer : framebuffer_resources.uniforms) {
                        if (attribute == uniform_buffer->description().attribute) {
                            return &uniform_buffer;
                        }
                    }
                    return nullptr;
                }();
                commitReferenceBuffer(cpu_uniform, *gpu_uniform);
            }
        }
    }
}

} // namespace wg

