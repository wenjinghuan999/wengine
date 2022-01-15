#include "platform/inc/platform.inc"
#include "gfx/gfx-constants.h"

namespace wg {

namespace gfx_formats {

std::string ToString(Format format) {
    return vk::to_string(static_cast<vk::Format>(format));
}

}

namespace gfx_queues {

const char* const QUEUE_NAMES[NUM_QUEUES] = {
    "graphics",
    "present",
    "transfer",
    "compute"
};

}

} // namespace wg