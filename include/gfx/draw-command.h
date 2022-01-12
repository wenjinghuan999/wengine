#pragma once

#include "common/common.h"
#include "gfx/gfx-pipeline.h"

#include <memory>

namespace wg {

class DrawCommand : public std::enable_shared_from_this<DrawCommand> {
public:
    virtual ~DrawCommand() = default;
    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::shared_ptr<GfxPipeline>& pipeline() const { return pipeline_; }
    virtual void setPipeline(const std::shared_ptr<GfxPipeline>& pipeline);
    DrawCommand& addUniformBuffer(const std::shared_ptr<UniformBufferBase>& uniform_buffer);
    void clearUniformBuffers();
    [[nodiscard]] bool valid() const;
protected:
    std::string name_;
    std::shared_ptr<GfxPipeline> pipeline_;
    // CPU data of draw command uniforms
    std::map<uniform_attributes::UniformAttribute, 
        std::shared_ptr<UniformBufferBase>> uniform_buffers_;
protected:
    DrawCommand(std::string name);
    friend class Gfx;
    struct Impl;
    virtual Impl* getImpl() = 0;
};

class SimpleDrawCommand : public DrawCommand {
public:
    static std::shared_ptr<DrawCommand> Create(
        std::string name, const std::shared_ptr<GfxPipeline>& pipeline
    );
    virtual ~SimpleDrawCommand() override = default;
protected:
    explicit SimpleDrawCommand(std::string name, const std::shared_ptr<GfxPipeline>& pipeline);
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    virtual DrawCommand::Impl* getImpl() override;
};


}