#include "gfx/gfx.h"
#include "gfx/gfx-constants.h"
#include "gfx/surface.h"
#include "gfx/render-target.h"
#include "common/logger.h"
#include "surface-private.h"
#include "gfx-private.h"
#include "render-target-private.h"
#include "gfx-constants-private.h"
#include "draw-command-private.h"
#include "platform/inc/window-private.h"

namespace {

[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

RenderTarget::RenderTarget(std::string name) : name_(std::move(name)) {}

std::shared_ptr<RenderTarget> Gfx::createRenderTarget(const std::shared_ptr<Window>& window) {
    auto surface = getWindowSurface(window);
    if (!surface) {
        logger().error(
            "Cannot create render target for window \"{}\" because no surface is created.",
            window->title()
        );
        return {};
    }

    return std::shared_ptr<RenderTarget>(new RenderTargetSurface(window->title(), surface));
}

RenderTargetSurface::RenderTargetSurface(std::string name, const std::shared_ptr<Surface>& surface)
    : RenderTarget(std::move(name)), surface_(surface) {

    impl_ = std::make_unique<Impl>();
    impl_->get_image_views = [weak_surface = std::weak_ptr<Surface>(surface_)]() {
        std::vector<vk::ImageView> image_views;
        if (auto surface = weak_surface.lock()) {
            if (auto* resources = surface->impl_->resources.data()) {
                image_views.reserve(resources->vk_image_views.size());
                for (auto& vk_image_view : resources->vk_image_views) {
                    image_views.push_back(*vk_image_view);
                }
            }
        }
        return image_views;
    };
}

bool RenderTargetSurface::preRendering(class Gfx& gfx) {
    if (surface_->resized_) {
        logger().info("Recreating surface resources because of resize.");
        recreateSurfaceResources(gfx);
        surface_->resized_ = false;
        return false;
    } else if (surface_->hidden_) {
        return false;
    }
    return true;
}

int RenderTargetSurface::acquireImage(Gfx& gfx) {
    if (auto* surface_resources = surface_->impl_->resources.data()) {
        if (auto* resources = impl_->resources.data()) {
            // Use acquireNextImageKHR instead of acquireNextImage2KHR
            // before figuring out how to use device mask properly
            auto[result, image_index] = (**resources->device).acquireNextImageKHR(
                *surface_resources->vk_swapchain,
                UINT64_MAX,
                *resources->image_available_semaphores[resources->current_frame_index]
            );
            if (result == vk::Result::eSuccess) {
                return static_cast<int>(image_index);
            } else if (result == vk::Result::eErrorOutOfDateKHR ||
                result == vk::Result::eSuboptimalKHR) {
                logger().info("Skip acquire image: result = {}.", vk::to_string(result));
                recreateSurfaceResources(gfx);
            }
        }
    }
    return -1;
}

void RenderTargetSurface::recreateSurfaceResources(Gfx& gfx) {
    gfx.waitDeviceIdle();

    auto extent = surface_->extent();
    if (extent.x() == 0 || extent.y() == 0) {
        logger().info("Skip recreating resources for hidden surface \"{}\".", surface_->window_title());
        surface_->hidden_ = true;
        return;
    }

    impl_->resources.reset();
    bool result = gfx.createWindowSurfaceResources(surface_);
    if (result) {
        gfx.createRenderTargetResources(shared_from_this());
        gfx.submitDrawCommands(shared_from_this());
    }
}

void RenderTargetSurface::finishImage(Gfx& gfx, int image_index) {
    if (auto* surfaces_resources = surface_->impl_->resources.data()) {
        if (image_index < 0 || image_index > static_cast<int>(surfaces_resources->vk_image_views.size())) {
            logger().error("Cannot finish image because image index is invalid.");
            return;
        }
        if (auto* resources = impl_->resources.data()) {
            auto wait_semaphores = std::array{ *resources->render_finished_semaphores[resources->current_frame_index] };
            auto swapchains = std::array{ *surfaces_resources->vk_swapchain };
            auto image_indices = std::array{ static_cast<uint32_t>(image_index) };
            auto present_info = vk::PresentInfoKHR{}
                .setWaitSemaphores(wait_semaphores)
                .setSwapchains(swapchains)
                .setImageIndices(image_indices);

            auto& present_queue = resources->queues[1];
            // Do not use VulkanHpp for present because it throws when result == VK_ERROR_OUT_OF_DATE_KHR
            // See https://github.com/KhronosGroup/Vulkan-Hpp/issues/274
            auto result =
                static_cast<vk::Result>(present_queue.vk_device->getDispatcher()->vkQueuePresentKHR(
                    static_cast<VkQueue>(present_queue.vk_queue), reinterpret_cast<const VkPresentInfoKHR*>(&present_info)
                ));

            if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
                logger().info("Skip present: result = {}.", vk::to_string(result));
                recreateSurfaceResources(gfx);
                return;
            } else if (result != vk::Result::eSuccess) {
                logger().error("Present error: {}.", vk::to_string(result));
                return;
            }
            resources->current_frame_index = (resources->current_frame_index + 1) % resources->max_frames_in_flight;
        }
    }
}

void Gfx::createRenderTargetResources(const std::shared_ptr<RenderTarget>& render_target) {

    render_target->impl_->resources.reset();
    logger().info("Creating resources for render target \"{}\".", render_target->name());

    auto& renderer = render_target->renderer_;
    if (!renderer) {
        logger().error("Cannot create render target resources because renderer is not available.");
        return;
    }

    if (!logical_device_) {
        logger().error("Cannot create render target resources because logical device is not available.");
        return;
    }
    waitDeviceIdle();

    auto[width, height] = render_target->extent();
    auto image_views = render_target->impl_->get_image_views();
    auto image_count = image_views.size();

    auto resources = std::make_unique<RenderTargetResources>();
    resources->device = &logical_device_->impl_->vk_device;

    // Render pass
    auto color_attachment = vk::AttachmentDescription{
        .format         = gfx_formats::ToVkFormat(render_target->format()),
        .samples        = vk::SampleCountFlagBits::e1,
        .loadOp         = vk::AttachmentLoadOp::eClear,
        .storeOp        = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout  = vk::ImageLayout::eUndefined,
        .finalLayout    = vk::ImageLayout::ePresentSrcKHR
    };
    auto attachments = std::array{ color_attachment };

    auto color_attachment_reference = vk::AttachmentReference{
        .attachment = 0,
        .layout     = vk::ImageLayout::eColorAttachmentOptimal
    };
    auto color_attachments = std::array{ color_attachment_reference };

    auto subpass_description = vk::SubpassDescription{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics
    }
        .setColorAttachments(color_attachments);
    auto subpasses = std::array{ subpass_description };
    auto dependencies = std::array{
        vk::SubpassDependency{
            .srcSubpass    = VK_SUBPASS_EXTERNAL,
            .dstSubpass    = 0,
            .srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .srcAccessMask = {},
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite
        }
    };

    auto render_pass_create_info = vk::RenderPassCreateInfo{}
        .setAttachments(attachments)
        .setSubpasses(subpasses)
        .setDependencies(dependencies);

    resources->render_pass =
        logical_device_->impl_->vk_device.createRenderPass(render_pass_create_info);

    // Queues
    for (auto queue_id : render_target->queues()) {
        auto& queue_info_array = logical_device_->impl_->queue_references[queue_id];
        if (!queue_info_array.empty()) {
            auto& queue_info_ref = queue_info_array[0]; // Choose first for now
            resources->queues.push_back(queue_info_ref);
            if (queue_id == gfx_queues::graphics && resources->graphics_queue_index < 0) {
                resources->graphics_queue_index = static_cast<int>(resources->queues.size()) - 1;
            }
        } else {
            logger().error("Cannot get queue \"{}\" from device.", gfx_queues::QUEUE_NAMES[queue_id]);
        }
    }

    // Semaphores & fences
    resources->max_frames_in_flight = std::max(1, static_cast<int>(image_count - 1));
    resources->current_frame_index = 0;
    resources->images_in_flight.resize(image_count);
    for (int i = 0; i < resources->max_frames_in_flight; ++i) {
        resources->image_available_semaphores.emplace_back(logical_device_->impl_->vk_device.createSemaphore({}));
        resources->render_finished_semaphores.emplace_back(logical_device_->impl_->vk_device.createSemaphore({}));
        resources->in_flight_fences.emplace_back(
            logical_device_->impl_->vk_device.createFence({ .flags = vk::FenceCreateFlagBits::eSignaled })
        );
    }

    // Framebuffers
    resources->framebuffer_resources.reserve(image_count);
    for (auto&& image_view : image_views) {
        auto render_target_attachments = std::array{ image_view };
        auto framebuffer_create_info = vk::FramebufferCreateInfo{
            .renderPass = *resources->render_pass,
            .width      = static_cast<uint32_t>(width),
            .height     = static_cast<uint32_t>(height),
            .layers     = 1
        }
            .setAttachments(render_target_attachments);

        resources->framebuffer_resources.emplace_back(
            RenderTargetFramebufferResources{
                .framebuffer = logical_device_->impl_->vk_device.createFramebuffer(framebuffer_create_info)
            }
        );
    }

    // Command buffers
    if (resources->graphics_queue_index >= 0) {
        auto command_buffer_allocate_info = vk::CommandBufferAllocateInfo{
            .commandPool = resources->queues[resources->graphics_queue_index].vk_command_pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = static_cast<uint32_t>(image_count)
        };
        // Do not use vk::raii::CommandBuffer because there might be compile errors on MSVC
        // Since we are allocating from the pool, we can destruct command buffers together.
        resources->command_buffers =
            (*logical_device_->impl_->vk_device).allocateCommandBuffers(command_buffer_allocate_info);

        for (size_t i = 0; i < image_count; ++i) {
            resources->framebuffer_resources[i].command_buffer = resources->command_buffers[i];
        }
    } else {
        logger().error("Cannot allocate command buffer because no graphics queue has been assigned to render target.");
    }

    // Framebuffer uniform buffers
    for (size_t i = 0; i < image_count; ++i) {
        for (auto&&[attribute, cpu_uniform] : renderer->uniform_buffers_) {
            auto& gpu_uniform = resources->framebuffer_resources[i].uniforms.emplace_back(
                UniformBufferBase::Create(attribute)
            );
            createUniformBufferResources(gpu_uniform);
            if (cpu_uniform->has_cpu_data()) {
                commitReferenceBuffer(cpu_uniform, gpu_uniform);
            }
        }
    }

    render_target->impl_->resources =
        logical_device_->impl_->render_target_resources.store(std::move(resources));
}

void Gfx::render(const std::shared_ptr<RenderTarget>& render_target) {

    if (!render_target->preRendering(*this)) {
        return;
    }

    auto& renderer = render_target->renderer_;
    if (!renderer) {
        logger().error("Cannot render because renderer is not valid!");
        return;
    }

    auto* resources = render_target->impl_->resources.data();
    if (!resources) {
        logger().error("Cannot render because render target resources is not valid!");
        return;
    }

    auto result = logical_device_->impl_->vk_device.waitForFences(
        { *resources->in_flight_fences[resources->current_frame_index] }, true, UINT64_MAX
    );
    if (result != vk::Result::eSuccess) {
        logger().error("Wait for fence error: {}", vk::to_string(result));
    }

    // Acquire image
    auto image_index = render_target->acquireImage(*this);
    auto image_count = resources->framebuffer_resources.size();
    if (image_index < 0) {
        return;
    } else if (image_index >= int(resources->command_buffers.size())) {
        logger().error("Cannot render because render target returned invalid image index.");
        return;
    }

    // Update uniforms
    for (auto it = renderer->dirty_framebuffer_uniforms_.begin();
         it != renderer->dirty_framebuffer_uniforms_.end(); ++it) {
        auto&&[attribute, mask] = *it;
        commitFramebufferUniformBuffers(render_target, attribute, image_index);
        mask |= (1 << image_index);
        if (mask == (1 << image_count) - 1) {
            it = renderer->dirty_framebuffer_uniforms_.erase(it);
        }
    }
    for (auto it = renderer->dirty_draw_command_uniforms_.begin();
         it != renderer->dirty_draw_command_uniforms_.end(); ++it) {
        auto&&[key, mask] = *it;
        auto&&[draw_command_index, attribute] = key;
        commitDrawCommandUniformBuffers(render_target, draw_command_index, attribute, image_index);
        mask |= (1 << image_index);
        if (mask == (1 << image_count) - 1) {
            it = renderer->dirty_draw_command_uniforms_.erase(it);
        }
    }

    // Wait for image in flight
    if (resources->images_in_flight[image_index]) {
        result = logical_device_->impl_->vk_device.waitForFences(
            { resources->images_in_flight[image_index] }, true, UINT64_MAX
        );
        if (result != vk::Result::eSuccess) {
            logger().error("Wait for fence error: {}", vk::to_string(result));
        }
    }

    // Submit
    auto wait_semaphores = std::array{ *resources->image_available_semaphores[resources->current_frame_index] };
    auto wait_stages = std::array<vk::PipelineStageFlags, 1>{
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    auto command_buffers = std::array{ resources->command_buffers[image_index] }; // use copy (not raii)
    auto signal_semaphores = std::array{ *resources->render_finished_semaphores[resources->current_frame_index] };

    auto fence = *resources->in_flight_fences[resources->current_frame_index];
    resources->images_in_flight[image_index] = fence;
    logical_device_->impl_->vk_device.resetFences({ fence });

    auto submit_info = vk::SubmitInfo{
    }
        .setWaitSemaphores(wait_semaphores)
        .setWaitDstStageMask(wait_stages)
        .setCommandBuffers(command_buffers)
        .setSignalSemaphores(signal_semaphores);

    if (resources->graphics_queue_index >= 0) {
        resources->queues[resources->graphics_queue_index].vk_queue.submit({ submit_info }, fence);
    } else {
        logger().error("Cannot render because no graphics queue has been assigned to render target.");
        return;
    }

    // Finish image
    render_target->finishImage(*this, image_index);
}

} // namespace wg