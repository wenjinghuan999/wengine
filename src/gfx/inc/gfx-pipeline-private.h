#pragma once

#include "platform/inc/platform.inc"
#include "gfx/gfx-pipeline.h"
#include "common/owned_resources.h"

namespace wg {

struct GfxPipelineResources {
    vk::raii::PipelineLayout pipeline_layout{nullptr};
    vk::raii::Pipeline pipeline{nullptr};
};

struct GfxPipeline::Impl {
    OwnedResourceHandle resource_handle;
};

}