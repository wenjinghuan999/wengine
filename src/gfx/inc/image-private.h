#pragma once

#include "platform/inc/platform.inc"
#include "gfx/image.h"
#include "common/owned-resources.h"
#include "gfx-buffer-private.h"

namespace wg {

struct ImageResources {
    vk::raii::Image image{ nullptr };
    vk::raii::ImageView image_view{ nullptr };
    vk::raii::Sampler sampler{ nullptr };
    
    uint32_t width{ 0 };
    uint32_t height{ 0 };
    vk::ImageLayout image_layout{};
    uint32_t queue_family_index{ 0 };
};

struct Image::Impl : public GfxMemoryBase::Impl {
    OwnedResourceHandle<ImageResources> resources;
};

} // namespace wg
