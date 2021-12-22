#pragma once

#include <memory>

#include "common/common.h"
#include "gfx/draw-command.h"

namespace wg {

class Renderer : public std::enable_shared_from_this<Renderer> {
public:
    [[nodiscard]] static std::shared_ptr<Renderer> Create();
     // Indicates that command buffer is ready to be drawn
    void addDrawCommand(const std::shared_ptr<DrawCommand>& draw_command) {
        draw_commands.push_back(draw_command);
    }
    void clearDrawCommands() {
        draw_commands.clear();
    }
    ~Renderer() = default;
protected:
    std::vector<std::shared_ptr<DrawCommand>> draw_commands;
protected:
    explicit Renderer();
    friend class Gfx;
    friend class RenderTarget;
};

}