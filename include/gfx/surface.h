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
    ~Surface();

    [[nodiscard]] std::shared_ptr<Window> window() const { return window_.lock(); }
    [[nodiscard]] Extent2D extent() const;
    [[nodiscard]] gfx_formats::Format format() const;
    [[nodiscard]] std::string window_title() const {
        auto w = window_.lock();
        return w ? w->title() : "";
    }

protected:
    std::weak_ptr<Window> window_;
    bool resized_{ false }; // indicating surface resources are invalid because of resizing
    bool hidden_{ false };  // indicating surface is hidden so we should not try to create resources
protected:
    friend class Gfx;
    friend class RenderTargetSurface;
    explicit Surface(const std::shared_ptr<Window>& window);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}