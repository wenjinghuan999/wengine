#pragma once

#include "common/common.h"
#include "gfx/gfx-pipeline.h"
#include "gfx/image.h"

#include <memory>

namespace wg {

namespace primitive_topologies {

enum PrimitiveTopology {
    point_list,
    line_list,
    line_strip,
    triangle_list,
    triangle_strip,
    triangle_fan,
    line_list_with_adjacency,
    line_strip_with_adjacency,
    triangle_list_with_adjacency,
    triangle_strip_with_adjacency,
    patch_list
};

} // namespace primitive_types

struct VertexBufferCombinedDescription {
    vertex_attributes::VertexAttribute attribute{ vertex_attributes::none };
    gfx_formats::Format format{ gfx_formats::none };
    uint32_t location{ 0 };
    uint32_t stride{ 0 };
    uint32_t offset{ 0 };
    size_t vertex_buffer_index{ 0 };
};

class DrawCommand : public std::enable_shared_from_this<DrawCommand> {
public:
    virtual ~DrawCommand() = default;

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::shared_ptr<GfxPipeline>& pipeline() const { return pipeline_; }
    virtual void setPipeline(const std::shared_ptr<GfxPipeline>& pipeline);
    [[nodiscard]] bool valid() const;

    void addVertexBuffer(const std::shared_ptr<VertexBufferBase>& vertex_buffer);
    void clearVertexBuffers();
    [[nodiscard]] const std::vector<std::shared_ptr<VertexBufferBase>>& vertex_buffers() const {
        return vertex_buffers_;
    }
    [[nodiscard]] size_t vertex_count() const;
    [[nodiscard]] std::vector<VertexBufferCombinedDescription> getVertexBufferCombinedDescriptions() const;

    void setIndexBuffer(const std::shared_ptr<IndexBuffer>& index_buffer);
    void clearIndexBuffer();
    [[nodiscard]] const std::shared_ptr<IndexBuffer>& index_buffer() const {
        return index_buffer_;
    }
    [[nodiscard]] bool draw_indexed() const;
    
    void setPrimitiveTopology(primitive_topologies::PrimitiveTopology primitive_topology) { 
        primitive_topology_ = primitive_topology;
    }
    [[nodiscard]] primitive_topologies::PrimitiveTopology primitive_topology() const { return primitive_topology_; }
    [[nodiscard]] size_t index_count() const;

    DrawCommand& addUniformBuffer(const std::shared_ptr<UniformBufferBase>& uniform_buffer);
    void clearUniformBuffers();

    DrawCommand& addSampler(uint32_t binding, const std::shared_ptr<Sampler>& sampler);
    void clearSamplers();

protected:
    std::string name_;
    std::shared_ptr<GfxPipeline> pipeline_;
    // Vertex and index buffer
    std::vector<std::shared_ptr<VertexBufferBase>> vertex_buffers_;
    std::shared_ptr<IndexBuffer> index_buffer_;
    primitive_topologies::PrimitiveTopology primitive_topology_{ primitive_topologies::triangle_list };
    // CPU data of draw command uniforms
    std::map<uniform_attributes::UniformAttribute,
        std::shared_ptr<UniformBufferBase>> uniform_buffers_;
    // binding => sampler 
    std::map<uint32_t, std::shared_ptr<Sampler>> samplers_;

protected:
    friend class Gfx;
    explicit DrawCommand(std::string name);
    struct Impl;
    virtual Impl* getImpl() = 0;
};

class SimpleDrawCommand : public DrawCommand {
public:
    static std::shared_ptr<DrawCommand> Create(
        std::string name, const std::shared_ptr<GfxPipeline>& pipeline
    );
    ~SimpleDrawCommand() override = default;

protected:
    friend class Gfx;
    explicit SimpleDrawCommand(std::string name, const std::shared_ptr<GfxPipeline>& pipeline);
    struct Impl;
    std::unique_ptr<Impl> impl_;
    DrawCommand::Impl* getImpl() override;
};

} // namespace wg