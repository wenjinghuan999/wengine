#pragma once

#include <functional>

#include "platform/inc/platform.inc"
#include "gfx/draw-command.h"

namespace wg {

struct DrawCommand::Impl {
    std::function<void(vk::CommandBuffer&)> draw;
};

}