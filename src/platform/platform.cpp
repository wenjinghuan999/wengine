#include "platform/platform.h"

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

namespace wg {

Monitor Monitor::getPrimary() {
    Monitor monitor;
    monitor.impl_ = glfwGetPrimaryMonitor();
    return monitor;
}

Window::Window(
    int width, int height, std::string title, 
    std::optional<Monitor> monitor, std::shared_ptr<Window> share
) : title_(std::move(title)) {

    GLFWmonitor* raw_monitor = monitor ? std::any_cast<GLFWmonitor*>(monitor->impl_) : nullptr;
    GLFWwindow* raw_share = share ? std::any_cast<GLFWwindow*>(share->impl_) : nullptr;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    impl_ = glfwCreateWindow(width, height, title_.c_str(), raw_monitor, raw_share);

    spdlog::info("Window created: \"{}\"({}x{})", title_, width, height);
}

Window::~Window() {
    GLFWwindow* raw_window = std::any_cast<GLFWwindow*>(impl_);
    
    int width = 0, height = 0;
    glfwGetWindowSize(raw_window, &width, &height);

    glfwDestroyWindow(raw_window);

    spdlog::info("Window destroyed: \"{}\"({}x{})", title_, width, height);
}

App::App() {
    spdlog::info("Initializing.");
    glfwInit();
}

App::~App() {
    spdlog::info("Terminating.");
    glfwTerminate();
}

void App::loop() {
    spdlog::info("Loop start.");

    while (!windows_.empty()) {
        for (auto it = windows_.begin(); it != windows_.end(); ) {
            GLFWwindow* raw_window = std::any_cast<GLFWwindow*>((*it)->impl_);
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
    
    spdlog::info("Loop end.");
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