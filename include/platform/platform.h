#pragma once

#include "common/common.h"

#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <tuple>

namespace wg {

class Monitor : public ICopyable {
public:
    static Monitor GetPrimary();
protected:
    Monitor() {}
    friend class Window;
    void* impl_{};
};

class Window : public IMovable {
public:
    Window(
        int width, int height, std::string title, 
        std::optional<Monitor> monitor = {}, std::shared_ptr<Window> share = {}
    );
    ~Window();
protected:
    std::string title_;
protected:
    friend class App;
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class App : IMovable {
public:
    App(std::string name, std::tuple<int, int, int> version);
    void loop();
    ~App();

    const std::string& name() const { return name_; }
    int major_version() const { return std::get<0>(version_); }
    int minor_version() const { return std::get<1>(version_); }
    int patch_version() const { return std::get<2>(version_); }
    std::string version_string() const;

    std::shared_ptr<Window> createWindow(
        int width, int height, std::string title, 
        std::optional<Monitor> monitor = {}, std::shared_ptr<Window> share = {}
    );
protected:
    std::vector<std::shared_ptr<Window>> windows_;
    const std::string name_;
    const std::tuple<int, int, int> version_;
};

}