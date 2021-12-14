#include "platform/platform.h"
#include "gfx/gfx.h"

int main(int, char**) {
    wg::App app("wegnine-gfx-example", std::make_tuple(0, 0, 1));
    auto window = app.createWindow(800, 600, "WEngine gfx example");

    wg::Gfx gfx(app);
    gfx.createWindowSurface(window);
    gfx.selectBestPhysicalDevice();
    gfx.createLogicalDevice();

    app.loop();
    gfx.destroyWindowSurface(window);

    return 0;
}
