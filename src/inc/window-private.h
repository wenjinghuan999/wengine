#include "platform/platform.h"
#include "common/owned_resources.h"
#include "platform-private.h"

namespace wg {

class Window;
struct Window::Impl {
    GLFWwindow* glfw_window{};
    OwnedResourceHandle surface_handle;
};

} // namespace wg