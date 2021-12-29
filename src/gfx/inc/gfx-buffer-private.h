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

namespace index_types {
    inline vk::IndexType ToVkIndexType(IndexType index_type) {
        return index_type == index_16 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;
    }

    inline IndexType ToVkIndexType(vk::IndexType vk_index_type) {
        return vk_index_type == vk::IndexType::eUint16 ? index_16 : index_32;
    }
}

}