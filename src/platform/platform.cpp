#include "platform.inc"
#include "platform/platform.h"
#include "common/logger.h"
#include "window-private.h"

#include <numeric>

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
    impl_->resources.reset();

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

void Window::SubPlotLayout(const Monitor& monitor, std::vector<std::shared_ptr<Window>> windows, int n_rows, int n_cols) {
    std::vector<float> widths(n_cols);
    std::vector<float> heights(n_rows);
    for (int i = 0; i < static_cast<int>(windows.size()); ++i) {
        int row = i / n_cols % n_rows;
        int col = i % n_cols;
        auto size = windows[i] ? windows[i]->total_size() : Size2D();
        widths[col] = std::max(widths[col], static_cast<float>(size.x()));
        heights[row] = std::max(heights[row], static_cast<float>(size.y()));
    }
    float total_width = std::accumulate(widths.begin(), widths.end(), 0.f);
    float total_height = std::accumulate(heights.begin(), heights.end(), 0.f);

    auto* glfw_monitor = reinterpret_cast<GLFWmonitor*>(monitor.impl_);
    int work_area_x_pos, work_area_y_pos, work_area_width, work_area_height;
    glfwGetMonitorWorkarea(glfw_monitor, &work_area_x_pos, &work_area_y_pos, &work_area_width, &work_area_height);
    
    float margin_w = std::min((static_cast<float>(work_area_width) - total_width) / static_cast<float>(n_cols - 1), 10.f);
    total_width += margin_w * static_cast<float>(n_cols - 1);
    total_width = std::min(total_width, static_cast<float>(work_area_width));
    float margin_h = std::min((static_cast<float>(work_area_height) - total_height) / static_cast<float>(n_rows - 1), 10.f);
    total_height += margin_h * static_cast<float>(n_rows - 1);
    total_height = std::min(total_height, static_cast<float>(work_area_height));

    std::vector<float> x_offsets(n_cols);
    std::vector<float> y_offsets(n_rows);
    for (size_t i = 0; i < x_offsets.size(); ++i) {
        x_offsets[i] = i == 0 ? 0.f : x_offsets[i - 1] + widths[i - 1] + margin_w;
    }
    for (size_t i = 0; i < y_offsets.size(); ++i) {
        y_offsets[i] = i == 0 ? 0.f : y_offsets[i - 1] + heights[i - 1] + margin_h;
    }

    for (int i = 0; i < static_cast<int>(windows.size()); ++i) {
        if (!windows[i]) {
            continue;
        }
        int row = i / n_cols % n_rows;
        int col = i % n_cols;
        auto size = windows[i]->total_size();
        float x_center = x_offsets[col] + widths[col] / 2;
        float y_center = y_offsets[row] + heights[row] / 2;
        int x_pos = static_cast<int>(x_center - static_cast<float>(size.x()) / 2 + (static_cast<float>(work_area_width) - total_width) / 2);
        int y_pos = static_cast<int>(y_center - static_cast<float>(size.y()) / 2 + (static_cast<float>(work_area_height) - total_height) / 2);
        windows[i]->setPosition({ work_area_x_pos + x_pos, work_area_y_pos + y_pos });
    }
}

void Window::setTitle(const std::string& title) {
    GLFWwindow* glfw_window = impl_->glfw_window;
    glfwSetWindowTitle(glfw_window, title.c_str());
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
            auto& window = *it;
            GLFWwindow* glfw_window = window->impl_->glfw_window;
            if (glfwWindowShouldClose(glfw_window)) {
                if (window->on_window_closed_) {
                    window->on_window_closed_();
                }
                window_data_.erase(window->weak_from_this());
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

void App::registerWindowData(const std::shared_ptr<Window>& window, std::any data) {
    window_data_[window->weak_from_this()].emplace_back(std::move(data));
}

} // namespace wg