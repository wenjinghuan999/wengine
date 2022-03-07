#include "platform.inc"
#include "platform/platform.h"
#include "common/logger.h"
#include "window-private.h"

namespace {

auto& logger() {
    static auto logger_ = wg::Logger::Get("platform");
    return *logger_;
}

} // unnamed namespace

namespace wg {

Monitor Monitor::GetPrimary() {
    Monitor monitor;
    monitor.impl_ = glfwGetPrimaryMonitor();
    return monitor;
}

Size2D Monitor::position() const {
    auto* glfw_monitor = reinterpret_cast<GLFWmonitor*>(impl_);
    
    int x_pos, y_pos, width, height;
    glfwGetMonitorWorkarea(glfw_monitor, &x_pos, &y_pos, &width, &height);
    return { x_pos, y_pos };
}

Size2D Monitor::work_area_size() const {
    auto* glfw_monitor = reinterpret_cast<GLFWmonitor*>(impl_);
    
    int x_pos, y_pos, width, height;
    glfwGetMonitorWorkarea(glfw_monitor, &x_pos, &y_pos, &width, &height);
    return { width, height };
}

Size2D Monitor::size() const {
    auto* glfw_monitor = reinterpret_cast<GLFWmonitor*>(impl_);

    auto mode = glfwGetVideoMode(glfw_monitor);
    return { mode->width, mode->height };
}

Window::Window(
    int width, int height, std::string title,
    const std::optional<Monitor>& monitor, window_mode::WindowMode mode
) : title_(std::move(title)), impl_(std::make_unique<Window::Impl>()) {

    GLFWmonitor* glfw_monitor = monitor ? static_cast<GLFWmonitor*>(monitor->impl_) : nullptr;
    if (!glfw_monitor) {
        mode = window_mode::windowed;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
    if (mode == window_mode::windowed) {
        impl_->glfw_window = glfwCreateWindow(width, height, title_.c_str(), nullptr, nullptr);
    } else if (mode == window_mode::windowed_fullscreen) {
        const GLFWvidmode* video_mode = glfwGetVideoMode(glfw_monitor); 
        glfwWindowHint(GLFW_RED_BITS, video_mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, video_mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, video_mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, video_mode->refreshRate);
        impl_->glfw_window = glfwCreateWindow(video_mode->width, video_mode->height, title_.c_str(), glfw_monitor, nullptr);
    } else {
        const GLFWvidmode* video_mode = glfwGetVideoMode(glfw_monitor);
        impl_->glfw_window = glfwCreateWindow(video_mode->width, video_mode->height, title_.c_str(), glfw_monitor, nullptr);
    }

    logger().info("Window created: \"{}\"({}x{}).", title_, width, height);
}

Window::~Window() {
    impl_->surface_handle.reset();

    GLFWwindow* glfw_window = impl_->glfw_window;
    auto[width, height] = this->extent();

    glfwDestroyWindow(glfw_window);

    logger().info("Window destroyed: \"{}\"({}x{}).", title_, width, height);
}

Size2D Window::extent() const {
    GLFWwindow* glfw_window = impl_->glfw_window;

    int width = 0, height = 0;
    glfwGetFramebufferSize(glfw_window, &width, &height);

    return { width, height };
}

Size2D Window::total_size() const {
    GLFWwindow* glfw_window = impl_->glfw_window;

    int left = 0, top = 0, right = 0, bottom = 0;
    glfwGetWindowFrameSize(glfw_window, &left, &top, &right, &bottom);
    int width = 0, height = 0;
    glfwGetWindowSize(glfw_window, &width, &height);

    return { width + left + right, height + top + bottom };
}

void Window::setPosition(Size2D position) {
    GLFWwindow* glfw_window = impl_->glfw_window;
    glfwSetWindowPos(glfw_window, position.x(), position.y());
}

void Window::setPositionToCenter(const Monitor& monitor) {
    GLFWwindow* glfw_window = impl_->glfw_window;
    GLFWmonitor* glfw_monitor = reinterpret_cast<GLFWmonitor*>(monitor.impl_);

    int width, height;
    glfwGetWindowSize(glfw_window, &width, &height);
    int left, top, right, bottom;
    glfwGetWindowFrameSize(glfw_window, &left, &top, &right, &bottom);
    int work_area_x_pos, work_area_y_pos, work_area_width, work_area_height;
    glfwGetMonitorWorkarea(glfw_monitor, &work_area_x_pos, &work_area_y_pos, &work_area_width, &work_area_height);
    
    int pos_x = work_area_x_pos + (work_area_width - width - left - right) / 2;
    int pos_y = work_area_y_pos + (work_area_height - height - left - right) / 2;
    
    glfwSetWindowPos(glfw_window, pos_x, pos_y);
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

void App::loop(const std::function<void(float)>& func) {
    logger().info("Event loop start.");
    static auto startTime = std::chrono::high_resolution_clock::now();

    while (!windows_.empty()) {
        for (auto it = windows_.begin(); it != windows_.end();) {
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

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        func(time);
        
        wait();
    }

    logger().info("Event loop end.");
}

void App::wait() {
    glfwPollEvents();
}

std::shared_ptr<Window> App::createWindow(
    int width, int height, const std::string& title,
    const std::optional<Monitor>& monitor, window_mode::WindowMode mode
) {
    std::shared_ptr<Window> window(new Window(width, height, title, monitor, mode));
    windows_.push_back(window);
    return window;
}

} // namespace wg