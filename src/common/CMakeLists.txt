add_library(wengine-logger INTERFACE)

target_sources(wengine-logger
    INTERFACE ${PROJECT_SOURCE_DIR}/include/common/logger.h
)

target_link_libraries(wengine-logger
    INTERFACE spdlog::spdlog
)

add_library(wengine-common INTERFACE)

target_link_libraries(wengine-common
    INTERFACE glm::glm
    INTERFACE fmt::fmt
)

target_sources(wengine-common
    INTERFACE ${PROJECT_SOURCE_DIR}/include/common/common.h
    INTERFACE ${PROJECT_SOURCE_DIR}/include/common/constants.h
    INTERFACE ${PROJECT_SOURCE_DIR}/include/common/math.h
    INTERFACE ${PROJECT_SOURCE_DIR}/include/common/owned-resources.h
    INTERFACE ${PROJECT_SOURCE_DIR}/include/common/singleton.h
)
