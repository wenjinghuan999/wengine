#pragma once

#include <functional>

#include "platform/inc/platform.inc"
#include "gfx/command-buffer.h"
#include "common/owned-resources.h"

namespace wg {

struct CommandBufferResources {
    std::vector<vk::CommandBuffer> command_buffers;
};

struct CommandBuffer::Impl {
    OwnedResourceHandle<CommandBufferResources> resources;
};

}