#pragma once

#include "common/common.h"
#include "common/math.h"
#include "platform/platform.h"
#include "gfx/renderer.h"
#include "gfx/surface.h"

#include <memory>

namespace wg {

class RenderTarget : public std::enable_shared_from_this<RenderTarget> {
public:
    virtual ~RenderTarget() = default;

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] virtual Size2D extent() const = 0;
    [[nodiscard]] virtual gfx_formats::Format format() const = 0;
    [[nodiscard]] virtual gfx_formats::Format depth_format() const = 0;
    [[nodiscard]] virtual std::vector<gfx_queues::QueueId> queues() const = 0;
    [[nodiscard]] virtual bool preRendering(class Gfx& gfx) = 0;
    [[nodiscard]] virtual int acquireImage(class Gfx& gfx) = 0;
    virtual void finishImage(class Gfx& gfx, int image_index) = 0;
    [[nodiscard]] std::shared_ptr<Renderer> renderer() const { return renderer_; }
    void setRenderer(const std::shared_ptr<Renderer>& renderer) { renderer_ = renderer; }

protected:
    std::string name_;
    std::shared_ptr<Renderer> renderer_;

protected:
    friend class Gfx;
    friend class RenderTargetSurface;
    explicit RenderTarget(std::string name);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class RenderTargetSurface : public RenderTarget {
public:
    Size2D extent() const override { return surface_->extent(); }
    gfx_formats::Format format() const override { return surface_->format(); }
    gfx_formats::Format depth_format() const override { return surface_->depth_format(); }
    std::vector<gfx_queues::QueueId> queues() const override {
        return { gfx_queues::graphics, gfx_queues::present };
    }
    bool preRendering(class Gfx& gfx) override;
    int acquireImage(class Gfx& gfx) override;
    void finishImage(class Gfx& gfx, int image_index) override;
    ~RenderTargetSurface() override = default;

protected:
    std::shared_ptr<Surface> surface_;
protected:
    friend class Gfx;
    friend class RenderTarget;
    RenderTargetSurface(std::string name, std::shared_ptr<Surface> surface);
    void recreateSurfaceResources(Gfx& gfx);
};

} // namespace wg