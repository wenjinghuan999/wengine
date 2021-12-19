#pragma once

#include "platform/inc/platform.inc"
#include "gfx/render-target.h"
#include "common/owned-resources.h"

namespace wg {

struct RenderTargetResources {
};

struct RenderTarget::Impl {
};

struct RenderTargetSurface::Impl : public RenderTarget::Impl {
};

}