add_library(wengine-logger INTERFACE)

target_sources(wengine-logger
    INTERFACE ${PROJECT_SOURCE_DIR}/include/common/logger.h)

target_link_libraries(wengine-logger
    INTERFACE spdlog::spdlog)

add_library(wengine-common
    config.cpp
    ${PROJECT_SOURCE_DIR}/include/common/common.h
    ${PROJECT_SOURCE_DIR}/include/common/config.h
    ${PROJECT_SOURCE_DIR}/include/common/constants.h
    ${PROJECT_SOURCE_DIR}/include/common/math.h
    ${PROJECT_SOURCE_DIR}/include/common/owned-resources.h
    ${PROJECT_SOURCE_DIR}/include/common/singleton.h)

add_dependencies(wengine-common wengine-config)

target_link_libraries(wengine-common
    INTERFACE glm::glm
    INTERFACE fmt::fmt 
    PRIVATE third-party-json)

target_include_directories(wengine-common
    PUBLIC ${PROJECT_SOURCE_DIR}/include)