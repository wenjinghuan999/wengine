#pragma once

#include "platform/inc/platform.inc"
#include "platform/platform.h"
#include "common/owned_resources.h"

namespace wg {

class Window;
struct Window::Impl {
    GLFWwindow* glfw_window{};
    OwnedResourceHandleUntyped surface_handle;
};

} // namespace wg