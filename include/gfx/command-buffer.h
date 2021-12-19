#pragma once

#include <memory>

#include "common/common.h"
#include "gfx/render-target.h"
#include "gfx/draw-command.h"

namespace wg {

class CommandBuffer : public std::enable_shared_from_this<CommandBuffer> {
public:
    [[nodiscard]] static std::shared_ptr<CommandBuffer> Create(const std::shared_ptr<RenderTarget>& render_target);
    [[nodiscard]] std::shared_ptr<RenderTarget> render_target() const { return render_target_; }
     // Indicates that command buffer is ready to be drawn
    [[nodiscard]] bool ready() const { return ready_; }
    void addDrawCommand(const std::shared_ptr<DrawCommand> draw_command) {
        draw_commands.push_back(draw_command);
        ready_ = false;
    }
    void clearDrawCommands() {
        draw_commands.clear();
        ready_ = false;
    }
    ~CommandBuffer() = default;
protected:
    bool ready_{false};
    std::shared_ptr<RenderTarget> render_target_;
    std::vector<std::shared_ptr<DrawCommand>> draw_commands;
protected:
    explicit CommandBuffer(const std::shared_ptr<RenderTarget>& render_target);
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}