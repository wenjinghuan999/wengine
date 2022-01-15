#pragma once

#include <map>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace wg {

class Logger {
public:
    static std::shared_ptr<spdlog::logger> Get(const std::string& label) {
        static std::map<std::string, std::shared_ptr<spdlog::logger>> loggers;

        auto it = loggers.find(label);
        if (it != loggers.end()) {
            return it->second;
        }

        auto logger = spdlog::stdout_color_mt(label);
#ifndef NDEBUG
        logger->set_level(spdlog::level::debug);
#endif
        loggers[label] = logger;
        return logger;
    }
};

} // namespace wg
