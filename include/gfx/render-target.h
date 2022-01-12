#pragma once

#include "common/common.h"
#include "common/math.h"
#include "platform/platform.h"
#include "gfx/renderer.h"

#include <memory>

namespace wg {

class RenderTarget : public std::enable_shared_from_this<RenderTarget> {
public:
    [[nodiscard]] const std::string name() const { return name_; }
    [[nodiscard]] virtual Extent2D extent() const = 0;
    [[nodiscard]] virtual gfx_formats::Format format() const = 0;
    [[nodiscard]] virtual std::vector<gfx_queues::QueueId> queues() const = 0;
    [[nodiscard]] virtual bool preRendering(class Gfx& gfx) = 0;
    [[nodiscard]] virtual int acquireImage(class Gfx& gfx) = 0;
    virtual void finishImage(class Gfx& gfx, int image_index) = 0;
    [[nodiscard]] std::shared_ptr<Renderer> renderer() const { return renderer_; }
    void setRenderer(const std::shared_ptr<Renderer>& renderer) { renderer_ = renderer; }
    virtual ~RenderTarget() = default;
protected:
    std::string name_;
    std::shared_ptr<Renderer> renderer_;
protected:
    RenderTarget(std::string name);
    friend class Gfx;
    friend class RenderTargetSurface;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class RenderTargetSurface : public RenderTarget {
public:
    virtual Extent2D extent() const override { return surface_->extent(); }
    virtual gfx_formats::Format format() const override { return surface_->format(); }
    virtual std::vector<gfx_queues::QueueId> queues() const override {
        return { gfx_queues::graphics, gfx_queues::present };
    }
    virtual bool preRendering(class Gfx& gfx) override;
    virtual int acquireImage(class Gfx& gfx) override;
    virtual void finishImage(class Gfx& gfx, int image_index) override;
    virtual ~RenderTargetSurface() override = default;
protected:
    RenderTargetSurface(std::string name, const std::shared_ptr<Surface>& surface);
    void recreateSurfaceResources(Gfx& gfx);
protected:
    friend class Gfx;
    friend class RenderTarget;
    std::shared_ptr<Surface> surface_;
};

}