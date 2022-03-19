#pragma once

#include "platform/inc/platform.inc"

#include "gfx/gfx-buffer.h"

#include "common/owned-resources.h"

namespace wg {

struct GfxMemoryResources {
    vk::raii::DeviceMemory memory{ nullptr };

    vk::MemoryPropertyFlags memory_properties;
};

struct GfxMemoryBase::Impl {
    OwnedResourceHandle<GfxMemoryResources> memory_resources;
};

struct GfxBufferResources {
    vk::raii::Buffer buffer{ nullptr };

    vk::DeviceSize cpu_data_size{ 0 };
    vk::SharingMode sharing_mode;
};

struct GfxBufferBase::Impl : public GfxMemoryBase::Impl {
    OwnedResourceHandle<GfxBufferResources> resources;
};

namespace index_types {

inline vk::IndexType ToVkIndexType(IndexType index_type) {
    return index_type == index_16 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;
}

inline IndexType FromVkIndexType(vk::IndexType vk_index_type) {
    return vk_index_type == vk::IndexType::eUint16 ? index_16 : index_32;
}

} // namespace index_types

} // namespace wg