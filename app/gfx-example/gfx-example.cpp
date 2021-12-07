#include "platform/platform.h"
#include "gfx/gfx.h"

int main(int, char**) {
    auto app = std::make_unique<wg::App>("wegnine-gfx-example", std::make_tuple(0, 0, 1));
    app->createWindow(800, 600, "WEngine gfx example");

    wg::Gfx gfx(*app);

    app->loop();

    return 0;
}
