add_library(wengine-logger INTERFACE)

target_sources(wengine-logger
    INTERFACE ${PROJECT_SOURCE_DIR}/include/common/logger.h
)

target_link_libraries(wengine-logger
    INTERFACE spdlog::spdlog
)
