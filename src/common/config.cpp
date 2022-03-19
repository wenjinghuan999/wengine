#include "common/config.h"

#include "nlohmann/json.hpp"

#include <fstream>

namespace wg {

struct Config::Impl {
    nlohmann::json json;
};

Config::Config(std::string filename) : filename_(std::move(filename)), impl_(std::make_unique<Impl>()) {}
Config::~Config() = default;

EngineConfig::EngineConfig() : Config("config/engine.json") { load(); }
EngineConfig::~EngineConfig() = default;

Config& Config::save() {
    std::ofstream out(filename_);
    if (out) {
        out << std::setw(4) << impl_->json << std::endl;
        dirty_ = false;
    }
    return *this;
}

Config& Config::load() {
    std::ifstream in(filename_);
    if (in) {
        in >> impl_->json;
    } else {
        impl_->json.clear();
    }
    dirty_ = false;
    return *this;
}

template <>
int Config::ConfigHelper<int>::get(Config::Impl* impl, const std::string& key) {
    if (impl->json.contains(key)) {
        auto item = impl->json[key];
        if (item.is_number_integer()) {
            return impl->json[key];
        }
    }
    return {};
}

template <>
float Config::ConfigHelper<float>::get(Config::Impl* impl, const std::string& key) {
    if (impl->json.contains(key)) {
        auto item = impl->json[key];
        if (item.is_number_float()) {
            return impl->json[key];
        }
    }
    return {};
}

template <>
bool Config::ConfigHelper<bool>::get(Config::Impl* impl, const std::string& key) {
    if (impl->json.contains(key)) {
        auto item = impl->json[key];
        if (item.is_boolean()) {
            return impl->json[key];
        }
    }
    return {};
}

template <>
std::string Config::ConfigHelper<std::string>::get(Config::Impl* impl, const std::string& key) {
    if (impl->json.contains(key)) {
        auto item = impl->json[key];
        if (item.is_string()) {
            return impl->json[key];
        }
    }
    return {};
}

template <typename T>
void Config::ConfigHelper<T>::set(Config::Impl* impl, const std::string& key, const T& value) {
    impl->json[key] = static_cast<T>(value);
}

template void Config::ConfigHelper<int>::set(Config::Impl*, const std::string&, const int&);
template void Config::ConfigHelper<float>::set(Config::Impl*, const std::string&, const float&);
template void Config::ConfigHelper<bool>::set(Config::Impl*, const std::string&, const bool&);
template void Config::ConfigHelper<std::string>::set(Config::Impl*, const std::string&, const std::string&);

} // namespace wg