#pragma once

#include <functional>

#include "platform/inc/platform.inc"
#include "gfx/renderer.h"
#include "common/owned-resources.h"

namespace wg {

struct RendererResources {
    std::vector<vk::CommandBuffer> command_buffers;
};

struct Renderer::Impl {
    OwnedResourceHandle<RendererResources> resources;
};

}