#pragma once

#include "platform/inc/platform.inc"
#include "gfx/gfx-constants.h"

namespace wg {

namespace gfx_formats {

[[nodiscard]] inline Format FromVkFormat(vk::Format vk_format) {
    return static_cast<Format>(vk_format);
}

[[nodiscard]] inline vk::Format ToVkFormat(Format format) {
    return static_cast<vk::Format>(format);
}

} // namespace gfx_formats

} // namespace wg