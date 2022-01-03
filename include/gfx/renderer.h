#pragma once

#include <memory>
#include <vector>

#include "common/common.h"
#include "common/math.h"
#include "gfx/draw-command.h"
#include "gfx/gfx-buffer.h"
#include "gfx/gfx-constants.h"

namespace wg {

class IRenderer : public IGfxObject {
public:
    virtual std::vector<std::shared_ptr<DrawCommand>> getDrawCommands() const = 0;
protected:
    IRenderer() = default;
    ~IRenderer() = default;
};

class BasicRenderer : public IRenderer, public std::enable_shared_from_this<BasicRenderer> {
public:
    [[nodiscard]] static std::shared_ptr<BasicRenderer> Create() {
        return std::shared_ptr<BasicRenderer>(new BasicRenderer());
    }
    virtual std::shared_ptr<IRenderData> createRenderData() override {
        return DummyRenderData::Create();
    }
    virtual std::vector<std::shared_ptr<DrawCommand>> getDrawCommands() const override {
        return draw_commands;
    }
    void addDrawCommand(const std::shared_ptr<DrawCommand>& draw_command) {
        draw_commands.push_back(draw_command);
    }
    void clearDrawCommands() {
        draw_commands.clear();
    }
    ~BasicRenderer() = default;
protected:
    std::vector<std::shared_ptr<DrawCommand>> draw_commands;
protected:
    BasicRenderer() = default;
    friend class Gfx;
    friend class RenderTarget;
};

}