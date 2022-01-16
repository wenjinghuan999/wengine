#include "platform/inc/platform.inc"
#include "gfx/gfx-constants.h"

namespace wg {

namespace gfx_formats {

std::string ToString(Format format) {
    return vk::to_string(static_cast<vk::Format>(format));
}

int GetChannels(Format format) {
    switch (format) {
    case R8Unorm:
    case R8Snorm:
    case R8Uscaled:
    case R8Sscaled:
    case R8Uint:
    case R8Sint:
    case R8Srgb:
        return 1;
    case R8G8B8Unorm:
    case R8G8B8Snorm:
    case R8G8B8Uscaled:
    case R8G8B8Sscaled:
    case R8G8B8Uint:
    case R8G8B8Sint:
    case R8G8B8Srgb:
    case B8G8R8Unorm:
    case B8G8R8Snorm:
    case B8G8R8Uscaled:
    case B8G8R8Sscaled:
    case B8G8R8Uint:
    case B8G8R8Sint:
    case B8G8R8Srgb:
        return 3;
    case R8G8B8A8Unorm:
    case R8G8B8A8Snorm:
    case R8G8B8A8Uscaled:
    case R8G8B8A8Sscaled:
    case R8G8B8A8Uint:
    case R8G8B8A8Sint:
    case R8G8B8A8Srgb:
    case B8G8R8A8Unorm:
    case B8G8R8A8Snorm:
    case B8G8R8A8Uscaled:
    case B8G8R8A8Sscaled:
    case B8G8R8A8Uint:
    case B8G8R8A8Sint:
    case B8G8R8A8Srgb:
        return 4;
    default:
        return 0;
    }   
}

} // namespace gfx_formats

namespace gfx_queues {

const char* const QUEUE_NAMES[NUM_QUEUES] = {
    "graphics",
    "present",
    "transfer",
    "compute"
};

} // namespace gfx_queues

} // namespace wg