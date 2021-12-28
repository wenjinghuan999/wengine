#pragma once

#include "platform/inc/platform.inc"
#include "gfx/vertex-buffer.h"
#include "common/owned-resources.h"

namespace wg {

struct VertexBufferResources {
    vk::raii::Buffer buffer{ nullptr };
    vk::raii::DeviceMemory memory{ nullptr };
};

struct VertexBuffer::Impl {
    OwnedResourceHandle<VertexBufferResources> resources;
};

}