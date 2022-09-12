#pragma once

#include "platform/inc/platform.inc"

#include "gfx/image.h"

#include "common/owned-resources.h"
#include "gfx-buffer-private.h"
#include "gfx-constants-private.h"

namespace wg {

namespace image_types {

[[nodiscard]] inline std::tuple<vk::ImageType, vk::ImageViewType> ToVkImageAndViewType(image_types::ImageType image_type) {
    if (image_type == image_types::image_cube) {
        return { vk::ImageType::e2D, vk::ImageViewType::eCube };
    }
    else {
        return { vk::ImageType::e2D, vk::ImageViewType::e2D };
    }
}

} // namespace image_types

namespace image_sampler {

[[nodiscard]] inline Filter FromVkFilter(vk::Filter vk_filter) {
    return static_cast<Filter>(vk_filter);
}

[[nodiscard]] inline vk::Filter ToVkFilter(Filter filter) {
    return static_cast<vk::Filter>(filter);
}

[[nodiscard]] inline MipMapMode FromVkMipMapMode(vk::SamplerMipmapMode vk_mip_map_mode) {
    return static_cast<MipMapMode>(vk_mip_map_mode);
}

[[nodiscard]] inline vk::SamplerMipmapMode ToVkMipMapMode(MipMapMode mip_map_mode) {
    return static_cast<vk::SamplerMipmapMode>(mip_map_mode);
}

[[nodiscard]] inline AddressMode FromVkAddressMode(vk::SamplerAddressMode vk_address_mode) {
    return static_cast<AddressMode>(vk_address_mode);
}

[[nodiscard]] inline vk::SamplerAddressMode ToVkAddressMode(AddressMode address_mode) {
    return static_cast<vk::SamplerAddressMode>(address_mode);
}

[[nodiscard]] inline BorderColor FromVkBorderColor(vk::BorderColor vk_border_color) {
    switch (vk_border_color) {
    case vk::BorderColor::eFloatTransparentBlack:
    case vk::BorderColor::eIntTransparentBlack:
        return transparent_black;
    case vk::BorderColor::eFloatOpaqueBlack:
    case vk::BorderColor::eIntOpaqueBlack:
        return opaque_black;
    case vk::BorderColor::eFloatOpaqueWhite:
    case vk::BorderColor::eIntOpaqueWhite:
        return opaque_white;
    default:
        return custom;
    }
}

[[nodiscard]] inline vk::BorderColor ToVkBorderColor(BorderColor border_color, bool integer) {
    switch (border_color) {
    case transparent_black:
        return integer ? vk::BorderColor::eIntTransparentBlack : vk::BorderColor::eFloatTransparentBlack;
    case opaque_black:
        return integer ? vk::BorderColor::eIntOpaqueBlack : vk::BorderColor::eFloatOpaqueBlack;
    case opaque_white:
        return integer ? vk::BorderColor::eIntOpaqueWhite : vk::BorderColor::eFloatOpaqueWhite;
    default:
        return integer ? vk::BorderColor::eIntCustomEXT : vk::BorderColor::eFloatCustomEXT;
    }
}

} // namespace image_sampler

struct ImageResources {
    vk::raii::Image image{ nullptr };
    vk::raii::ImageView image_view{ nullptr };
    std::vector<vk::raii::Semaphore> ownership_transfer_semaphores;
    std::vector<vk::raii::Fence> ownership_transfer_fences;

    uint32_t width{ 0 };
    uint32_t height{ 0 };
    uint32_t mip_levels{ 0 };
    uint32_t layer_count{ 1 };
    vk::Format format;
    vk::ImageLayout image_layout{};
    QueueInfoRef queue;
};

struct Image::Impl : public GfxMemoryBase::Impl {
    OwnedResourceHandle<ImageResources> resources;
};

struct SamplerResources {
    vk::raii::Sampler sampler{ nullptr };
};

struct Sampler::Impl {
    OwnedResourceHandle<SamplerResources> resources;
};

} // namespace wg
