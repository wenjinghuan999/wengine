#include "platform/inc/platform.inc"
#include "gfx/gfx-constants.h"

namespace wg {

namespace gfx_formats {
    std::string ToString(Format format) {
        return vk::to_string(static_cast<vk::Format>(format));
    }
}

} // namespace wg