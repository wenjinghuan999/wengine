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
}

} // namespace wg