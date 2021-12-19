#include "gfx/gfx.h"
#include "gfx/gfx-constants.h"
#include "gfx/surface.h"
#include "gfx/render-target.h"
#include "common/logger.h"
#include "surface-private.h"
#include "gfx-private.h"
#include "render-target-private.h"
#include "gfx-constants-private.h"
#include "platform/inc/window-private.h"

namespace {
    
[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

std::shared_ptr<RenderTarget> Gfx::createRenderTarget(const std::shared_ptr<Window>& window) const {
    if (auto surface = getWindowSurface(window)) {
        return std::shared_ptr<RenderTarget>(new RenderTargetSurface(surface));
    } else {
        return {};
    }
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

} // namespace wg