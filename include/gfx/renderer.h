#pragma once

#include <memory>

#include "common/common.h"
#include "gfx/render-target.h"
#include "gfx/draw-command.h"

namespace wg {

class Renderer : public std::enable_shared_from_this<Renderer> {
public:
    [[nodiscard]] static std::shared_ptr<Renderer> Create(const std::shared_ptr<RenderTarget>& render_target);
    [[nodiscard]] std::shared_ptr<RenderTarget> render_target() const { return render_target_; }
     // Indicates that command buffer is ready to be drawn
    void addDrawCommand(const std::shared_ptr<DrawCommand> draw_command) {
        draw_commands.push_back(draw_command);
        render_target()->markDirty();
    }
    void clearDrawCommands() {
        draw_commands.clear();
        render_target()->markDirty();
    }
    ~Renderer() = default;
protected:
    std::shared_ptr<RenderTarget> render_target_;
    std::vector<std::shared_ptr<DrawCommand>> draw_commands;
protected:
    explicit Renderer(const std::shared_ptr<RenderTarget>& render_target);
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}