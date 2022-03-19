#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <map>
#include <string>

namespace wg {

class Logger {
public:
    static std::shared_ptr<spdlog::logger> Get(const std::string& label) {
        static std::map<std::string, std::shared_ptr<spdlog::logger>> loggers;

        auto it = loggers.find(label);
        if (it != loggers.end()) {
            auto logger = it->second;
            if (log_level_ != spdlog::level::info) {
                logger->set_level(log_level_);
            }
            return logger;
        }

        auto logger = spdlog::stdout_color_mt(label);
#ifndef NDEBUG
        logger->set_level(spdlog::level::debug);
#endif
        if (log_level_ != spdlog::level::info) {
            logger->set_level(log_level_);
        }

        loggers[label] = logger;
        return logger;
    }

    static void Disable() {
        log_level_ = spdlog::level::off;
    }

protected:
    static spdlog::level::level_enum log_level_;
};

} // namespace wg
