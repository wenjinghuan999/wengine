#pragma once

#include "common/common.h"
#include "common/owned-resources.h"
#include "gfx/gfx-buffer.h"
#include "gfx/shader.h"

#include <cinttypes>
#include <initializer_list>
#include <memory>
#include <utility>

namespace wg {

struct VertexFactoryDescription {
    vertex_attributes::VertexAttribute attribute{ vertex_attributes::none };
    gfx_formats::Format format{ gfx_formats::none };
    uint32_t location{ 0 };
};

class GfxVertexFactory {
public:
    GfxVertexFactory() = default;
    GfxVertexFactory(std::initializer_list<VertexFactoryDescription> initializer_list);

    void addDescription(VertexFactoryDescription description);
    void clearDescriptions();
    [[nodiscard]] const std::vector<VertexFactoryDescription>& descriptions() const { return descriptions_; }

protected:
    std::vector<VertexFactoryDescription> descriptions_;

protected:
    friend class Gfx;
};

struct GfxPipelineState {
    float min_sample_shading = 0.f;
};

struct UniformDescription {
    uniform_attributes::UniformAttribute attribute;
    uint32_t binding; // use std::numeric_limits<uint32_t>::max() for push constants
    shader_stages::ShaderStages stages;
    uint32_t push_constant_offset;
    uint32_t push_constant_size;
};

class GfxUniformLayout {
public:
    GfxUniformLayout& addDescription(UniformDescription description);
    void clearDescriptions();
    [[nodiscard]] const std::vector<UniformDescription>& descriptions() const { return descriptions_; }
    [[nodiscard]] const UniformDescription* getDescription(uniform_attributes::UniformAttribute attribute) const;

protected:
    std::vector<UniformDescription> descriptions_;

protected:
    friend class Gfx;
};

struct SamplerDescription {
    uint32_t binding;
    shader_stages::ShaderStages stages;
};

class GfxSamplerLayout {
public:
    GfxSamplerLayout& addDescription(SamplerDescription description);
    void clearDescriptions();
    [[nodiscard]] const std::vector<SamplerDescription>& descriptions() const { return descriptions_; }

protected:
    std::vector<SamplerDescription> descriptions_;

protected:
    friend class Gfx;
};

class GfxPipeline : public std::enable_shared_from_this<GfxPipeline> {
public:
    static std::shared_ptr<GfxPipeline> Create();
    ~GfxPipeline() = default;

public:
    [[nodiscard]] const GfxVertexFactory& vertex_factory() const { return vertex_factory_; }
    [[nodiscard]] const GfxPipelineState& pipeline_state() const { return pipeline_state_; }
    [[nodiscard]] const GfxUniformLayout& uniform_layout() const { return uniform_layout_; }
    [[nodiscard]] const GfxSamplerLayout& sampler_layout() const { return sampler_layout_; }
    [[nodiscard]] const std::vector<std::shared_ptr<Shader>>& shaders() const { return shaders_; }

    void setVertexFactory(GfxVertexFactory vertex_factory) { vertex_factory_ = std::move(vertex_factory); }
    void setPipelineState(GfxPipelineState pipeline_state) { pipeline_state_ = pipeline_state; }
    void setUniformLayout(GfxUniformLayout uniform_layout) { uniform_layout_ = std::move(uniform_layout); }
    void setSamplerLayout(GfxSamplerLayout sampler_layout) { sampler_layout_ = std::move(sampler_layout); }
    void addShader(const std::shared_ptr<Shader>& shader) { shaders_.push_back(shader); }
    void clearShaders() { shaders_.clear(); }

protected:
    GfxVertexFactory vertex_factory_;
    GfxPipelineState pipeline_state_;
    GfxUniformLayout uniform_layout_;
    GfxSamplerLayout sampler_layout_;
    std::vector<std::shared_ptr<Shader>> shaders_;

protected:
    friend class Gfx;
    friend class DrawCommand;
    GfxPipeline();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace wg