#pragma once

#include "common/common.h"
#include "gfx/gfx-pipeline.h"

#include <memory>

namespace wg {

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
    const std::vector<std::shared_ptr<VertexBufferBase>>& vertex_buffers() const {
        return vertex_buffers_;
    }
    size_t vertex_count() const;
    std::vector<VertexBufferCombinedDescription> getVertexBufferCombinedDescriptions() const;

    void setIndexBuffer(const std::shared_ptr<IndexBuffer>& index_buffer);
    void clearIndexBuffer();
    const std::shared_ptr<IndexBuffer>& index_buffer() const {
        return index_buffer_;
    }
    bool draw_indexed() const;
    size_t index_count() const;

    DrawCommand& addUniformBuffer(const std::shared_ptr<UniformBufferBase>& uniform_buffer);
    void clearUniformBuffers();

protected:
    std::string name_;
    std::shared_ptr<GfxPipeline> pipeline_;
    // Vertex and index buffer
    std::vector<std::shared_ptr<VertexBufferBase>> vertex_buffers_;
    std::shared_ptr<IndexBuffer> index_buffer_;
    // CPU data of draw command uniforms
    std::map<uniform_attributes::UniformAttribute,
        std::shared_ptr<UniformBufferBase>> uniform_buffers_;

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

}