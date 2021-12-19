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
    impl_->get_image_views = [weak_surface=std::weak_ptr<Surface>(surface_)]() {
        std::vector<vk::ImageView> image_views;
        if (auto surface = weak_surface.lock()) {
            if (auto resources = surface->impl_->resources->get()) {
                image_views.reserve(resources->vk_image_views.size());
                for (auto& vk_image_view : resources->vk_image_views) {
                    image_views.push_back(*vk_image_view);
                }
            }
        }
        return image_views;
    };
}

void Gfx::createRenderTargetResources(const std::shared_ptr<RenderTarget>& render_target) {

    render_target->impl_->resources.reset();

    if (!logical_device_) {
        logger().error("Cannot create render target resources because logical device is not available.");
    }

    auto [width, height] = render_target->extent();
    auto image_views = render_target->impl_->get_image_views();

    auto resources = std::make_unique<RenderTargetResources>();
    
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
        .layout = vk::ImageLayout::eColorAttachmentOptimal
    };
    auto color_attachments = std::array{ color_attachment_reference };

    auto subpass_description = vk::SubpassDescription{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics
    }   .setColorAttachments(color_attachments);
    auto subpasses = std::array{subpass_description};

    auto render_pass_create_info = vk::RenderPassCreateInfo{}
        .setAttachments(attachments)
        .setSubpasses(subpasses);
    
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
            .layers      = 1
        }   .setAttachments(render_target_attachments);
        resources->framebuffers.emplace_back(
            logical_device_->impl_->vk_device.createFramebuffer(framebuffer_create_info));
    }

    render_target->impl_->resources = 
        logical_device_->impl_->render_target_resources.store(std::move(resources));
}

} // namespace wg