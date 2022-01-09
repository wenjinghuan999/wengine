#pragma once

#include "common/common.h"
#include "gfx/gfx-pipeline.h"

#include <memory>

namespace wg {

class DrawCommand : public std::enable_shared_from_this<DrawCommand> {
public:
    const std::shared_ptr<GfxPipeline>& pipeline() const { return pipeline_; }
    virtual ~DrawCommand() = default;
    virtual void setPipeline(const std::shared_ptr<GfxPipeline>& pipeline);
protected:
    std::shared_ptr<GfxPipeline> pipeline_;
protected:
    DrawCommand() = default;
    friend class Gfx;
    struct Impl;
    virtual Impl* getImpl() = 0;
};

class SimpleDrawCommand : public DrawCommand {
public:
    static std::shared_ptr<DrawCommand> Create(
        const std::shared_ptr<GfxPipeline>& pipeline
    );
    virtual ~SimpleDrawCommand() override = default;
protected:
    explicit SimpleDrawCommand(const std::shared_ptr<GfxPipeline>& pipeline);
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    virtual DrawCommand::Impl* getImpl() override;
};


}