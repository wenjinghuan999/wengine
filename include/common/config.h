#pragma once

#include "common/singleton.h"

#include <map>
#include <string>

namespace wg {

class Config : public Singleton<Config> {
public:
    std::string& operator[](const std::string& key) {
        return values_[key];
    }
    [[nodiscard]] bool get_bool(const std::string& key) const {
        auto it = values_.find(key);
        if (it == values_.end()) {
            return false;
        }
        if (it->second.empty() ||
            it->second == "0" ||
            it->second == "false") {
            return false;
        }
        return true;
    }
    [[nodiscard]] std::string get_string(const std::string& key) const {
        auto it = values_.find(key);
        if (it == values_.end()) {
            return "";
        }
        return it->second;
    }
    void save();
    void load();
    
protected:
    std::string filename_{ "config/config.json" };
    std::map<std::string, std::string> values_;

protected:
    friend class Singleton<Config>;
};

} // namespace wg
