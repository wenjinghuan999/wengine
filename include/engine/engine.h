#pragma once

#include <string>

#include "common/singleton.h"

namespace wg {

class Engine : public Singleton<Engine> {
    friend class Singleton<Engine>;
public:
    const std::string& name() const { return name_; }
    int major_version() const { return std::get<0>(version_); }
    int minor_version() const { return std::get<1>(version_); }
    int patch_version() const { return std::get<2>(version_); }
    std::string version_string() const;
protected:
    Engine();
protected:
    std::string name_;
    std::tuple<int, int, int> version_;
};

}