#include "common/config.h"
#include "nlohmann/json.hpp"

#include <fstream>

namespace wg {

void Config::save() {
    nlohmann::json json = values_;
    std::ofstream out(filename_);
    out << std::setw(4) << json << std::endl;
}

void Config::load() {
    std::ifstream in(filename_);
    nlohmann::json json;
    in >> json;
    values_ = json;
}

}// namespace wg