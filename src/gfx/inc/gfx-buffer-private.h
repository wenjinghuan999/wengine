#pragma once

#include "platform/inc/platform.inc"
#include "gfx/gfx-buffer.h"
#include "common/owned-resources.h"

namespace wg {

struct BufferResources {
    vk::raii::Buffer buffer{ nullptr };
    vk::raii::DeviceMemory memory{ nullptr };
};

struct GfxBufferBase::Impl {
    OwnedResourceHandle<BufferResources> resources;
};

}