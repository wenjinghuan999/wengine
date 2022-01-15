add_library(wengine-platform
    platform.cpp
    ${PROJECT_SOURCE_DIR}/include/platform/platform.h
    ${PROJECT_SOURCE_DIR}/src/platform/inc/platform.inc
    ${PROJECT_SOURCE_DIR}/src/platform/inc/window-private.h)

target_include_directories(wengine-platform
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${PROJECT_SOURCE_DIR}/src
    PUBLIC ${PROJECT_SOURCE_DIR}/src/platform/inc)

target_link_libraries(wengine-platform
    PUBLIC wengine-common
    PUBLIC wengine-logger

    PRIVATE Vulkan::Vulkan
    PRIVATE glfw
    PRIVATE glm::glm)
