add_library(wengine-platform
    platform.cpp
    ${PROJECT_SOURCE_DIR}/include/platform/platform.h
    ${PROJECT_SOURCE_DIR}/src/inc/platform-private.h
)

target_include_directories(wengine-platform
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${PROJECT_SOURCE_DIR}/src/inc
)

target_link_libraries(wengine-platform
    PRIVATE Vulkan::Vulkan
    PRIVATE glfw
    PRIVATE glm::glm
    PRIVATE spdlog::spdlog
)
