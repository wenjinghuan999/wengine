#pragma once

#include <string>

#include "common/singleton.h"

namespace wg {

class Engine : public Singleton<Engine> {
    friend class Singleton<Engine>;
public:
    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] int major_version() const { return std::get<0>(version_); }
    [[nodiscard]] int minor_version() const { return std::get<1>(version_); }
    [[nodiscard]] int patch_version() const { return std::get<2>(version_); }
    [[nodiscard]] std::string version_string() const;
protected:
    Engine();
protected:
    std::string name_;
    std::tuple<int, int, int> version_;
};

}