#pragma once

#include <functional>

#include "platform/inc/platform.inc"
#include "gfx/draw-command.h"
#include "gfx/gfx-pipeline.h"
#include "gfx-pipeline-private.h"

namespace wg {

struct DrawCommand::Impl {
    GfxPipelineResources* pipeline_resources;
    virtual void draw(vk::CommandBuffer& command_buffer) = 0;
};

struct SimpleDrawCommand::Impl : public DrawCommand::Impl {
    virtual void draw(vk::CommandBuffer& command_buffer) override;
};

}