#pragma once

#include "common/common.h"
#include "common/math.h"

#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <tuple>

namespace wg {

namespace window_mode {

enum WindowMode {
    windowed = 0,
    windowed_fullscreen,
    fullscreen
};

} // namespace window_mode


class Monitor : public ICopyable {
public:
    [[nodiscard]] static Monitor GetPrimary();
    
    [[nodiscard]] Size2D position() const;
    [[nodiscard]] Size2D work_area_size() const;
    [[nodiscard]] Size2D size() const;

protected:
    Monitor() = default;
    friend class Window;
    void* impl_{};
};

class Window : public std::enable_shared_from_this<Window> {
public:
    ~Window();

    [[nodiscard]] const std::string& title() const { return title_; }
    [[nodiscard]] Size2D extent() const;
    [[nodiscard]] Size2D total_size() const;
    
    void setPosition(Size2D position);
    void setPositionToCenter(const Monitor& monitor);
    static void SubPlotLayout(const Monitor& monitor, std::vector<std::shared_ptr<Window>> windows, int n_rows, int n_cols);

protected:
    std::string title_;

protected:
    friend class App;
    friend class Gfx;
    friend class Surface;
    Window(
        int width, int height, std::string title,
        const std::optional<Monitor>& monitor = {}, window_mode::WindowMode mode = window_mode::windowed
    );
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class App : public IMovable {
public:
    App(std::string name, std::tuple<int, int, int> version);
    ~App() override;

    void loop(const std::function<void(float)>& func);
    void wait();

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] int major_version() const { return std::get<0>(version_); }
    [[nodiscard]] int minor_version() const { return std::get<1>(version_); }
    [[nodiscard]] int patch_version() const { return std::get<2>(version_); }
    [[nodiscard]] std::string version_string() const;

    [[nodiscard]] std::shared_ptr<Window> createWindow(
        int width, int height, const std::string& title,
        const std::optional<Monitor>& monitor = {}, window_mode::WindowMode mode = window_mode::windowed
    );

protected:
    std::vector<std::shared_ptr<Window>> windows_;
    const std::string name_;
    const std::tuple<int, int, int> version_;
};

}