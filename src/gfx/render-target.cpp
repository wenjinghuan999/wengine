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

std::shared_ptr<RenderTarget> Gfx::createRenderTarget(const std::shared_ptr<Window>& window) {
    auto surface = getWindowSurface(window);
    if (!surface) {
        logger().error("Cannot create render target for window \"{}\" because no surface is created.",
            window->title());
        return {};
    }
    
    auto render_target = std::shared_ptr<RenderTarget>(new RenderTargetSurface(surface));
    createRenderTargetResources(render_target);
    return render_target;
}

RenderTargetSurface::RenderTargetSurface(const std::shared_ptr<Surface>& surface)
    : surface_(surface) {
    
    impl_ = std::make_unique<Impl>();
    impl_->get_image_views = [
            weak_surface=std::weak_ptr<Surface>(surface_)
        ]() {
        std::vector<vk::ImageView> image_views;
        if (auto surface = weak_surface.lock()) {
            if (auto* resources = surface->impl_->resources.get()) {
                image_views.reserve(resources->vk_image_views.size());
                for (auto& vk_image_view : resources->vk_image_views) {
                    image_views.push_back(*vk_image_view);
                }
            }
        }
        return image_views;
    };
}

int RenderTargetSurface::acquireImage(Gfx& gfx) {
    if (auto* surface_resources = surface_->impl_->resources.get()) {
        if (auto* resources = impl_->resources.get()) {
            // Use acquireNextImageKHR instead of acquireNextImage2KHR
            // before figuring out how to use device mask properly
            auto [result, image_index] = (**resources->device).acquireNextImageKHR(
                *surface_resources->vk_swapchain,
                UINT64_MAX,
                *resources->image_available_semaphores[resources->current_image_index]
            );
            if (result == vk::Result::eSuccess) {
                return static_cast<int>(image_index);
            } else if (surface_->resized_ ||
                result == vk::Result::eErrorOutOfDateKHR || 
                result == vk::Result::eSuboptimalKHR) {
                recreateSurfaceResources(gfx);
            }
        }
    }
    return -1;
}

void RenderTargetSurface::recreateSurfaceResources(Gfx& gfx) {
    gfx.waitDeviceIdle();
    impl_->resources.reset();
    gfx.createWindowSurfaceResources(surface_);
    gfx.createRenderTargetResources(shared_from_this());
    gfx.submitDrawCommands(shared_from_this());
}

void RenderTargetSurface::finishImage(int image_index) {
    if (auto* surfaces_resources = surface_->impl_->resources.get()) {
        if (image_index < 0 || image_index > static_cast<int>(surfaces_resources->vk_image_views.size())) {
            logger().error("Cannot finish image because image index is invalid.");
            return;
        }
        if (auto* resources = impl_->resources.get()) {
            auto wait_semaphores = std::array{ *resources->render_finished_semaphores[resources->current_image_index] };
            auto swapchains = std::array{ *surfaces_resources->vk_swapchain };
            auto image_indices = std::array{ static_cast<uint32_t>(image_index) };
            auto present_info = vk::PresentInfoKHR{}
                .setWaitSemaphores(wait_semaphores)
                .setSwapchains(swapchains)
                .setImageIndices(image_indices);
            
            auto* present_queue = resources->queues[1];
            auto result = present_queue->vk_queue.presentKHR(present_info);
            if (result != vk::Result::eSuccess) {
                logger().error("Present error: {}.", vk::to_string(result));
            }
            resources->current_image_index = (resources->current_image_index + 1) % resources->image_available_semaphores.size();
        }
    }
}

void Gfx::createRenderTargetResources(const std::shared_ptr<RenderTarget>& render_target) {

    render_target->impl_->resources.reset();

    if (!logical_device_) {
        logger().error("Cannot create render target resources because logical device is not available.");
        return;
    }
    waitDeviceIdle();

    auto [width, height] = render_target->extent();
    auto image_views = render_target->impl_->get_image_views();

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
    }   .setColorAttachments(color_attachments);
    auto subpasses = std::array{subpass_description};
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
    
    // Framebuffers
    resources->framebuffers.reserve(image_views.size());
    for (auto&& image_view : image_views) {
        auto render_target_attachments = std::array{ image_view };
        auto framebuffer_create_info = vk::FramebufferCreateInfo{
            .renderPass = *resources->render_pass,
            .width      = static_cast<uint32_t>(width),
            .height     = static_cast<uint32_t>(height),
            .layers     = 1
        }   .setAttachments(render_target_attachments);
        resources->framebuffers.emplace_back(
            logical_device_->impl_->vk_device.createFramebuffer(framebuffer_create_info));
    }

    // Queues
    for (auto queue_id : render_target->queues()) {
        auto& queue_info_array = logical_device_->impl_->queues[queue_id];
        if (!queue_info_array.empty()) {
            auto& queue_info = queue_info_array[0]; // Choose first for now
            resources->queues.push_back(queue_info.get());
            if (queue_id == gfx_queues::graphics && !resources->graphics_queue) {
                resources->graphics_queue = queue_info.get();
            }
        } else {
            logger().error("Cannot get queue \"{}\" from device.", 
                gfx_queues::QUEUE_NAMES[queue_id]);
        }
    }

    // Semaphores & fences
    for (auto&& image_view : image_views) {
        resources->image_available_semaphores.emplace_back(logical_device_->impl_->vk_device.createSemaphore({}));
        resources->render_finished_semaphores.emplace_back(logical_device_->impl_->vk_device.createSemaphore({}));
        resources->image_idle_fences.emplace_back(logical_device_->impl_->vk_device.createFence({
            .flags = vk::FenceCreateFlagBits::eSignaled
        }));
    }

    // Command buffers
    if (resources->graphics_queue) {
        resources->command_buffers.resize(image_views.size());
        auto command_buffer_allocate_info = vk::CommandBufferAllocateInfo{
            .commandPool = *resources->graphics_queue->vk_command_pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = static_cast<uint32_t>(image_views.size())
        };
        // Do not use vk::raii::CommandBuffer because there might be compile errors on MSVC
        // Since we are allocating from the pool, we can destruct command buffers together.
        resources->command_buffers = 
            (*logical_device_->impl_->vk_device).allocateCommandBuffers(command_buffer_allocate_info);
    } else {
        logger().error("Cannot allocate command buffer because no graphics queue has been assigned to render target.");
    }

    render_target->impl_->resources = 
        logical_device_->impl_->render_target_resources.store(std::move(resources));
}

void Gfx::render(const std::shared_ptr<RenderTarget>& render_target)
{
    auto* resources = render_target->impl_->resources.get();
    if (!resources) {
        logger().error("Cannot render because render target resources is not valid!");
        return;
    }
    
    auto image_index = render_target->acquireImage(*this);
    if (image_index < 0) {
        return;
    } else if (image_index >= int(resources->command_buffers.size())) {
        logger().error("Cannot render because render target returned invalid image index.");
        return;
    }
    auto result = logical_device_->impl_->vk_device.waitForFences(
        { *resources->image_idle_fences[image_index] }, true, UINT64_MAX);
    if (result != vk::Result::eSuccess) {
        logger().error("Wait for fence error: {}", vk::to_string(result));
    }

    auto wait_semaphores = std::array{ *resources->image_available_semaphores[resources->current_image_index] };
    auto wait_stages = std::array<vk::PipelineStageFlags, 1>{
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    auto command_buffers = std::array{ resources->command_buffers[image_index] }; // use copy (not raii)
    auto signal_semaphores = std::array{ *resources->render_finished_semaphores[resources->current_image_index] };
    auto fence = *resources->image_idle_fences[image_index];
    logical_device_->impl_->vk_device.resetFences({ fence });

    auto submit_info = vk::SubmitInfo{
    }   .setWaitSemaphores(wait_semaphores)
        .setWaitDstStageMask(wait_stages)
        .setCommandBuffers(command_buffers)
        .setSignalSemaphores(signal_semaphores);
    
    if (resources->graphics_queue) {
        resources->graphics_queue->vk_queue.submit({ submit_info }, fence);
    } else {
        logger().error("Cannot render because no graphics queue has been assigned to render target.");
        return;
    }

    render_target->finishImage(image_index);
}

} // namespace wg