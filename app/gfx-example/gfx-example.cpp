#include "platform/platform.h"
#include "gfx/gfx.h"

int main(int, char**) {
    auto app = std::make_unique<wg::App>();
    app->createWindow(800, 600, "WEngine gfx test");

    wg::Gfx gfx;
    gfx.logExtensions();

    app->loop();

    return 0;
}
