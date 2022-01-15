#pragma once

#include <string>

#include "common/singleton.h"

namespace wg {

class EngineConstants : public Singleton<EngineConstants> {
public:
    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] int major_version() const { return std::get<0>(version_); }
    [[nodiscard]] int minor_version() const { return std::get<1>(version_); }
    [[nodiscard]] int patch_version() const { return std::get<2>(version_); }
    [[nodiscard]] std::string version_string() const {
        return fmt::format("v{}.{}.{}", major_version(), minor_version(), patch_version());
    }

protected:
    std::string name_;
    std::tuple<int, int, int> version_;

protected:
    friend class Singleton<EngineConstants>;
    EngineConstants() : name_("WEngine"), version_(0, 0, 1) {}
};

}