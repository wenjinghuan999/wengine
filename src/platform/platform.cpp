#include "platform/platform.h"
#include "common/logger.h"
#include "platform-private.h"

namespace {

auto& logger() {
    static auto logger_ = wg::Logger::Get("platform");
    return *logger_;
}

}

namespace wg {

Monitor Monitor::GetPrimary() {
    Monitor monitor;
    monitor.impl_ = glfwGetPrimaryMonitor();
    return monitor;
}

Window::Window(
    int width, int height, std::string title, 
    const std::optional<Monitor>& monitor, const std::shared_ptr<Window>& share
) : title_(std::move(title)), impl_(std::make_unique<Window::Impl>()) {

    GLFWmonitor* glfw_monitor = monitor ? static_cast<GLFWmonitor*>(monitor->impl_) : nullptr;
    GLFWwindow* glfw_window_share = share ? share->impl_->glfw_window : nullptr;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    impl_->glfw_window = glfwCreateWindow(width, height, title_.c_str(), glfw_monitor, glfw_window_share);

    logger().info("Window created: \"{}\"({}x{}).", title_, width, height);
}

Window::~Window() {
    impl_->surface_handle.reset();

    GLFWwindow* glfw_window = impl_->glfw_window;
    
    int width = 0, height = 0;
    glfwGetWindowSize(glfw_window, &width, &height);

    glfwDestroyWindow(glfw_window);

    logger().info("Window destroyed: \"{}\"({}x{}).", title_, width, height);
}

App::App(std::string name, std::tuple<int, int, int> version)
    : name_(std::move(name)), version_(std::move(version)) {

    logger().info("Initializing: {} {}.", this->name(), version_string());
    glfwInit();
}

App::~App() {
    logger().info("Terminating {}.", name());
    glfwTerminate();
}

std::string App::version_string() const {
    return fmt::format("v{}.{}.{}", major_version(), minor_version(), patch_version());
}

void App::loop() {
    logger().info("Event loop start.");

    while (!windows_.empty()) {
        for (auto it = windows_.begin(); it != windows_.end(); ) {
            GLFWwindow* glfw_window = (*it)->impl_->glfw_window;
            if (glfwWindowShouldClose(glfw_window)) {
                it = windows_.erase(it);
            } else {
                ++it;
            }
        }
        if (windows_.empty()) {
            break;
        }

        glfwPollEvents();
    }
    
    logger().info("Event loop end.");
}


std::shared_ptr<Window> App::createWindow(
    int width, int height, const std::string& title, 
    const std::optional<Monitor>& monitor, const std::shared_ptr<Window>& share
){
    std::shared_ptr<Window> window(new Window(width, height, title, monitor, share));
    windows_.push_back(window);
    return window;
}

}