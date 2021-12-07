#include <fmt/format.h>

#include "engine/engine.h"

namespace wg {

Engine::Engine()
    : name_("WEngine"), version_(0, 0, 1) {
}

std::string Engine::version_string() const {
    return fmt::format("v{}.{}.{}", major_version(), minor_version(), patch_version());
}

}