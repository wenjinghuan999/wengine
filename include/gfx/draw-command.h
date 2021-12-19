#pragma once

#include "common/common.h"
#include "gfx/gfx-pipeline.h"

#include <memory>

namespace wg {

class DrawCommand : public std::enable_shared_from_this<DrawCommand> {
public:
    const std::shared_ptr<GfxPipeline>& pipeline() const { return pipeline_; }
    virtual ~DrawCommand() = default;
protected:
    std::shared_ptr<GfxPipeline> pipeline_;
protected:
    explicit DrawCommand(const std::shared_ptr<GfxPipeline>& pipeline);
    friend class Gfx;
    friend class RenderTarget;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class SimpleDrawCommand : public DrawCommand {
public:
    static std::shared_ptr<DrawCommand> Create(
        const std::shared_ptr<GfxPipeline>& pipeline
    ) {
        return std::shared_ptr<DrawCommand>(new SimpleDrawCommand(pipeline));
    }
    virtual ~SimpleDrawCommand() override = default;
protected:
    explicit SimpleDrawCommand(const std::shared_ptr<GfxPipeline>& pipeline);
};


}