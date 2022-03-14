#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <fmt/format.h>

#include "platform.inc"
#include "common/math.h"
#include "common/logger.h"
#include "platform/platform.h"

namespace glm {

doctest::String toString(const glm::vec4& v) {
    return fmt::format("({}, {}, {}, {})", v.x, v.y, v.z, v.w).c_str();
}

}

TEST_CASE("glm") {
    glm::mat4 matrix(
        1.f, 2.f, 3.f, 4.f,
        5.f, 6.f, 7.f, 8.f,
        1.f, 2.f, 3.f, 4.f,
        5.f, 6.f, 7.f, 8.f
    );
    glm::vec4 vec(1.f, 2.f, 3.f, 4.f);
    glm::vec4 result = matrix * vec;
    glm::vec4 answer(34.f, 44.f, 54.f, 64.f);
    CHECK(result == answer);
}

namespace {

auto& logger() {
    static auto logger_ = wg::Logger::Get("platform");
    return *logger_;
}

} // unnamed namespace

template <> struct fmt::formatter<wg::Size2D> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
        
    auto format(wg::Size2D c, format_context& ctx) {
        return fmt::format_to(ctx.out(), "({},{})", c.x(), c.y());
    }
};

TEST_CASE("glfw") {
    auto app = wg::App::Create("wegnine-gfx-example-anisotropy", std::make_tuple(0, 0, 1));

    int width = 800, height = 600;
    auto window = app->createWindow(width, height, "wengine platform test");

    auto monitor = wg::Monitor::GetPrimary();
    auto monitor_work_area_size = monitor.work_area_size();
    logger().info("monitor work area size: {}", monitor_work_area_size);
    auto monitor_size = monitor.size();
    logger().info("monitor total size: {}", monitor_size);
    auto framebuffer_size = window->extent();
    logger().info("window framebuffer size: {}", framebuffer_size);
    auto window_size = window->total_size();
    logger().info("window total size: {}", window_size);
}