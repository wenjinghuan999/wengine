#pragma once

#include "common/common.h"
#include "common/math.h"
#include "platform/platform.h"
#include "gfx/gfx.h"

#include <memory>

namespace wg {

class RenderTarget : public std::enable_shared_from_this<RenderTarget> {
public:
    [[nodiscard]] virtual Extent2D extent() const = 0;
    [[nodiscard]] virtual gfx_formats::Format format() const = 0;
    [[nodiscard]] bool ready() const { return ready_; }
    [[nodiscard]] virtual std::vector<gfx_queues::QueueId> queues() const = 0;
    [[nodiscard]] virtual int acquireImage() = 0;
    virtual void finishImage(int image_index) = 0;
    void markDirty() { ready_ = false; }
    virtual ~RenderTarget() = default;
protected:
    bool ready_{false};
protected:
    RenderTarget() = default;
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
    virtual int acquireImage() override;
    virtual void finishImage(int image_index) override;
    virtual ~RenderTargetSurface() override = default;
protected:
    RenderTargetSurface(const std::shared_ptr<Surface>& surface);
protected:
    friend class Gfx;
    friend class RenderTarget;
    std::shared_ptr<Surface> surface_;
};

}