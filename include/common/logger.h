#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace wg {

class Logger {
public:
    static auto Get(const std::string& label) {
        auto logger = spdlog::stdout_color_mt(label);
#ifndef NDEBUG
        logger->set_level(spdlog::level::debug);
#endif
        return logger;
    }
};

}
