#pragma once

#include <memory>

#include "common/common.h"
#include "common/math.h"
#include "gfx/gfx-constants.h"
#include "platform/platform.h"

namespace wg {

class Surface : public std::enable_shared_from_this<Surface> {
public:
    [[nodiscard]] static std::shared_ptr<Surface> Create(const std::shared_ptr<Window>& window);
    [[nodiscard]] std::shared_ptr<Window> window() const { return window_.lock(); }
    [[nodiscard]] Extent2D extent() const;
    [[nodiscard]] gfx_formats::Format format() const;
    ~Surface() = default;
protected:
    std::weak_ptr<Window> window_;
protected:
    explicit Surface(const std::shared_ptr<Window>& window);
    friend class Gfx;
    friend class RenderTargetSurface;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}