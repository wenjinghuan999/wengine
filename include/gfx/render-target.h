#pragma once

#include "common/common.h"
#include "common/math.h"
#include "platform/platform.h"
#include "gfx/gfx.h"

#include <memory>

namespace wg {

class RenderTarget : public std::enable_shared_from_this<RenderTarget> {
public:
    static std::shared_ptr<RenderTarget> CreateFromWindowSurface(const Gfx& gfx, const std::shared_ptr<Window>& window);
    virtual Extent2D extent() const = 0;
    virtual gfx_formats::Format format() const = 0;
protected:
    RenderTarget() = default;
    ~RenderTarget() = default;
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class RenderTargetSurface : public RenderTarget {
public:
    virtual Extent2D extent() const override { return surface_->extent(); }
    virtual gfx_formats::Format format() const override { return surface_->format(); }
protected:
    RenderTargetSurface(const std::shared_ptr<Surface>& surface);
protected:
    friend class Gfx;
    friend class RenderTarget;
    struct Impl;
    std::shared_ptr<Surface> surface_;
};

}