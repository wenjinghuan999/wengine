#pragma once

#include <memory>
#include <vector>

#include "common/common.h"
#include "common/math.h"
#include "gfx/draw-command.h"
#include "gfx/gfx-buffer.h"
#include "gfx/gfx-constants.h"

namespace wg {

class Renderer : public IGfxObject {
public:
    virtual const std::vector<std::shared_ptr<DrawCommand>>& getDrawCommands() const = 0;
    Renderer& addUniformBuffer(const std::shared_ptr<UniformBufferBase>& uniform_buffer);
    void clearUniformBuffers();
    bool valid() const;
protected:
    friend class Gfx;
    // CPU data of framebuffer uniforms
    std::map<uniform_attributes::UniformAttribute,
        std::shared_ptr<UniformBufferBase>> uniform_buffers_;
protected:
    Renderer() = default;
    ~Renderer() = default;
};

class BasicRenderer : public Renderer, public std::enable_shared_from_this<BasicRenderer> {
public:
    [[nodiscard]] static std::shared_ptr<BasicRenderer> Create() {
        return std::shared_ptr<BasicRenderer>(new BasicRenderer());
    }
    virtual std::shared_ptr<IRenderData> createRenderData() override {
        return DummyRenderData::Create();
    }
    virtual const std::vector<std::shared_ptr<DrawCommand>>& getDrawCommands() const override {
        return draw_commands_;
    }
    void addDrawCommand(const std::shared_ptr<DrawCommand>& draw_command) {
        draw_commands_.push_back(draw_command);
    }
    void clearDrawCommands() {
        draw_commands_.clear();
    }
    ~BasicRenderer() = default;
protected:
    std::vector<std::shared_ptr<DrawCommand>> draw_commands_;
protected:
    BasicRenderer() = default;
    friend class Gfx;
    friend class RenderTarget;
};

}