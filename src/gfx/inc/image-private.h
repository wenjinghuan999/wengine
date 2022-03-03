#pragma once

#include "platform/inc/platform.inc"
#include "gfx/image.h"
#include "common/owned-resources.h"
#include "gfx-buffer-private.h"
#include "gfx-constants-private.h"

namespace wg {

struct ImageResources {
    vk::raii::Image image{ nullptr };
    vk::raii::ImageView image_view{ nullptr };
    
    uint32_t width{ 0 };
    uint32_t height{ 0 };
    vk::Format format;
    vk::ImageLayout image_layout{};
    QueueInfoRef queue;
};

struct SamplerResources {
    vk::raii::Sampler sampler{ nullptr };
};

struct Image::Impl : public GfxMemoryBase::Impl {
    OwnedResourceHandle<ImageResources> resources;
    OwnedResourceHandle<SamplerResources> sampler_resources;
};

} // namespace wg
