#pragma once

#include "common/common.h"
#include "common/owned-resources.h"
#include "gfx/gfx-buffer.h"
#include "gfx/shader.h"

#include <cinttypes>
#include <initializer_list>
#include <memory>

namespace wg {

struct VertexFactoryDescription {
    vertex_attributes::VertexAttribute attribute;
    gfx_formats::Format format;
    uint32_t location;
};

struct VertexFactoryCombinedDescription {
    vertex_attributes::VertexAttribute attribute;
    gfx_formats::Format format;
    uint32_t location;
    uint32_t stride;
    uint32_t offset;
    size_t vertex_buffer_index;
};

class GfxVertexFactory {
public:
    GfxVertexFactory() = default;
    GfxVertexFactory(std::initializer_list<VertexFactoryDescription> initializer_list);
    void addDescription(VertexFactoryDescription description);
    void clearDescriptions();
    void addVertexBuffer(const std::shared_ptr<VertexBufferBase>& vertex_buffer);
    void clearVertexBuffers();
    void setIndexBuffer(const std::shared_ptr<IndexBuffer>& index_buffer);
    void clearIndexBuffer();
    bool valid() const;
    bool draw_indexed() const;
    size_t vertex_count() const;
    size_t index_count() const;
    std::vector<VertexFactoryCombinedDescription> getCombinedDescriptions() const;
protected:
    std::vector<VertexFactoryDescription> descriptions_;
    std::vector<std::shared_ptr<VertexBufferBase>> vertex_buffers_;
    std::shared_ptr<IndexBuffer> index_buffer_;
protected:
    friend class Gfx;
    static void AddDescriptionImpl(std::vector<VertexFactoryDescription>& attributes, VertexFactoryDescription attribute);
    static void AddDescriptionImpl(std::vector<VertexBufferDescription>& attributes, VertexBufferDescription attribute);
};

class GfxPipelineState {

};

struct UniformDescription {
    uniform_attributes::UniformAttribute attribute;
    uint32_t binding;
    shader_stages::ShaderStages stages;
};

class GfxUniformLayout {
public:
    GfxUniformLayout& addDescription(UniformDescription description);
    void clearDescriptions();
    const std::vector<UniformDescription>& descriptions() const { return descriptions_; }
protected:
    friend class Gfx;
    std::vector<UniformDescription> descriptions_;
};

class GfxPipeline : public std::enable_shared_from_this<GfxPipeline> {
public:
    static std::shared_ptr<GfxPipeline> Create();
    ~GfxPipeline() = default;

public:
    [[nodiscard]] const GfxVertexFactory& vertex_factory() const { return vertex_factory_; }
    [[nodiscard]] const GfxPipelineState& pipeline_state() const { return pipeline_state_; }
    [[nodiscard]] const GfxUniformLayout& uniform_layout() const { return uniform_layout_; }
    [[nodiscard]] const std::vector<std::shared_ptr<Shader>>& shaders() const { return shaders_; }

    void setVertexFactory(GfxVertexFactory vertex_factory) { vertex_factory_ = vertex_factory; }
    void setPipelineState(GfxPipelineState pipeline_state) { pipeline_state_ = pipeline_state; }
    void setUniformLayout(GfxUniformLayout uniform_layout) { uniform_layout_ = uniform_layout; }
    void addShader(const std::shared_ptr<Shader>& shader) { shaders_.push_back(shader); }
    void setShaders(std::vector<std::shared_ptr<Shader>> shaders) { shaders_ = std::move(shaders); }
    void clearShaders() { shaders_.clear(); }
protected:
    GfxVertexFactory vertex_factory_;
    GfxPipelineState pipeline_state_;
    GfxUniformLayout uniform_layout_;
    std::vector<std::shared_ptr<Shader>> shaders_;
protected:
    GfxPipeline();
    friend class Gfx;
    friend class DrawCommand;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}