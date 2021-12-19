#pragma once

#include "common/common.h"
#include "common/owned-resources.h"
#include "gfx/render-target.h"

#include <memory>

namespace wg {

class GfxVertexFactory {

};

class GfxPipelineState {

};

class GfxUniformLayout {

};

class GfxPipeline : public std::enable_shared_from_this<GfxPipeline> {
public:
    static std::shared_ptr<GfxPipeline> Create(std::string name);
    ~GfxPipeline() = default;

public:
    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const GfxVertexFactory& vertex_factroy() const { return vertex_factory_; }
    [[nodiscard]] const GfxPipelineState& pipeline_state() const { return pipeline_state_; }
    [[nodiscard]] const GfxUniformLayout& uniform_layout() const { return uniform_layout_; }
    [[nodiscard]] const std::vector<std::shared_ptr<Shader>>& shaders() const { return shaders_; }
    [[nodiscard]] const std::shared_ptr<RenderTarget>& render_target() const { return render_target_; }

    void setVertexFactroy(GfxVertexFactory vertex_factory) { vertex_factory_ = vertex_factory; valid_ = false; }
    void setPipelineState(GfxPipelineState pipeline_state) { pipeline_state_ = pipeline_state; valid_ = false; }
    void setUniformLayout(GfxUniformLayout uniform_layout) { uniform_layout_ = uniform_layout; valid_ = false; }
    void addShader(const std::shared_ptr<Shader>& shader) { shaders_.push_back(shader); valid_ = false; }
    void setShaders(std::vector<std::shared_ptr<Shader>> shaders) { shaders_ = std::move(shaders); valid_ = false; }
    void clearShaders() { shaders_.clear(); valid_ = false; }
    void setRenderTarget(const std::shared_ptr<RenderTarget>& render_target) { render_target_ = render_target; }

    [[nodiscard]] bool valid() const { return valid_; }
protected:
    std::string name_;
    GfxVertexFactory vertex_factory_;
    GfxPipelineState pipeline_state_;
    GfxUniformLayout uniform_layout_;
    std::vector<std::shared_ptr<Shader>> shaders_;
    std::shared_ptr<RenderTarget> render_target_;
    bool valid_{false};
protected:
    explicit GfxPipeline(std::string name);
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}