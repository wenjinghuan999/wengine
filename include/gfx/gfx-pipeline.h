#pragma once

#include "common/common.h"
#include "common/owned-resources.h"

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

    void setVertexFactroy(GfxVertexFactory vertex_factory) { vertex_factory_ = vertex_factory; }
    void setPipelineState(GfxPipelineState pipeline_state) { pipeline_state_ = pipeline_state; }
    void setUniformLayout(GfxUniformLayout uniform_layout) { uniform_layout_ = uniform_layout; }
    void addShader(const std::shared_ptr<Shader>& shader) { shaders_.push_back(shader); }
    void setShaders(std::vector<std::shared_ptr<Shader>> shaders) { shaders_ = std::move(shaders); }
    void clearShaders() { shaders_.clear(); }
protected:
    std::string name_;
    GfxVertexFactory vertex_factory_;
    GfxPipelineState pipeline_state_;
    GfxUniformLayout uniform_layout_;
    std::vector<std::shared_ptr<Shader>> shaders_;
protected:
    explicit GfxPipeline(std::string name);
    friend class Gfx;
};

}