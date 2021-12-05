#pragma once

#include "common/common.h"

#include <string>
#include <memory>
#include <any>
#include <vector>
#include <optional>

namespace wg {

class Monitor : public ICopyable {
public:
    static Monitor getPrimary();
protected:
    friend class Window;
    std::any impl_;
};

class Window : public IMovable {
public:
    Window(
        int width, int height, std::string title, 
        std::optional<Monitor> monitor = {}, std::shared_ptr<Window> share = {}
    );
    ~Window();
protected:
    friend class App;
    std::string title_;
    std::any impl_;
};

class App : IMovable {
public:
    App();
    void loop();
    ~App();

    std::shared_ptr<Window> createWindow(
        int width, int height, std::string title, 
        std::optional<Monitor> monitor = {}, std::shared_ptr<Window> share = {}
    );
protected:
    std::vector<std::shared_ptr<Window>> windows_;
};

}