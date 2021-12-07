#include "platform/platform.h"

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace {
    auto logger = spdlog::stdout_color_mt("platform");
}

namespace wg {

Monitor Monitor::getPrimary() {
    Monitor monitor;
    monitor.impl_ = glfwGetPrimaryMonitor();
    return monitor;
}

struct Window::Impl {
    GLFWwindow* raw_window{};
};

Window::Window(
    int width, int height, std::string title, 
    std::optional<Monitor> monitor, std::shared_ptr<Window> share
) : title_(std::move(title)), impl_(std::make_unique<Window::Impl>()) {

    GLFWmonitor* raw_monitor = monitor ? static_cast<GLFWmonitor*>(monitor->impl_) : nullptr;
    GLFWwindow* raw_share = share ? share->impl_->raw_window : nullptr;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    impl_->raw_window = glfwCreateWindow(width, height, title_.c_str(), raw_monitor, raw_share);

    logger->info("Window created: \"{}\"({}x{}).", title_, width, height);
}

Window::~Window() {
    GLFWwindow* raw_window = impl_->raw_window;
    
    int width = 0, height = 0;
    glfwGetWindowSize(raw_window, &width, &height);

    glfwDestroyWindow(raw_window);

    logger->info("Window destroyed: \"{}\"({}x{}).", title_, width, height);
}

App::App(std::string name, std::tuple<int, int, int> version)
    : name_(std::move(name)), version_(std::move(version)) {

    logger->info("Initializing: {} {}.", this->name(), version_string());
    glfwInit();
}

App::~App() {
    logger->info("Terminating {}.", name());
    glfwTerminate();
}

std::string App::version_string() const {
    return fmt::format("v{}.{}.{}", major_version(), minor_version(), patch_version());
}

void App::loop() {
    logger->info("Event loop start.");

    while (!windows_.empty()) {
        for (auto it = windows_.begin(); it != windows_.end(); ) {
            GLFWwindow* raw_window = (*it)->impl_->raw_window;
            if (glfwWindowShouldClose(raw_window)) {
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
    
    logger->info("Event loop end.");
}


std::shared_ptr<Window> App::createWindow(
    int width, int height, std::string title, 
    std::optional<Monitor> monitor, std::shared_ptr<Window> share
){
    std::shared_ptr<Window> window = std::make_shared<Window>(width, height, title, monitor, share);
    windows_.push_back(window);
    return window;
}

}