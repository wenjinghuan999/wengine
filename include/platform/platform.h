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

class Monitor : public ICopyable {
public:
    [[nodiscard]] static Monitor GetPrimary();
protected:
    Monitor() = default;
    friend class Window;
    void* impl_{};
};

class Window : public std::enable_shared_from_this<Window> {
public:
    ~Window();
    [[nodiscard]] const std::string& title() const { return title_; }
    [[nodiscard]] Extent2D extent() const;
protected:
    Window(
        int width, int height, std::string title, 
        const std::optional<Monitor>& monitor = {}, const std::shared_ptr<Window>& share = {}
    );
protected:
    std::string title_;
protected:
    friend class App;
    friend class Gfx;
    friend class Surface;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class App : IMovable {
public:
    App(std::string name, std::tuple<int, int, int> version);
    void loop(const std::function<void(float)>& func);
    ~App() override;

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] int major_version() const { return std::get<0>(version_); }
    [[nodiscard]] int minor_version() const { return std::get<1>(version_); }
    [[nodiscard]] int patch_version() const { return std::get<2>(version_); }
    [[nodiscard]] std::string version_string() const;

    [[nodiscard]] std::shared_ptr<Window> createWindow(
        int width, int height, const std::string& title, 
        const std::optional<Monitor>& monitor = {}, const std::shared_ptr<Window>& share = {}
    );
protected:
    std::vector<std::shared_ptr<Window>> windows_;
    const std::string name_;
    const std::tuple<int, int, int> version_;
};

}