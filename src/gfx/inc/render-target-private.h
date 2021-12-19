#pragma once

#include <functional>

#include "platform/inc/platform.inc"
#include "gfx/render-target.h"
#include "common/owned-resources.h"

namespace wg {

struct RenderTarget::Impl {
    std::function<std::vector<vk::ImageView>()> get_image_views;
};

}