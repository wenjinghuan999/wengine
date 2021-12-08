add_library(wengine-platform
platform.cpp
    ${PROJECT_SOURCE_DIR}/include/platform/platform.h
)

target_include_directories(wengine-platform
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(wengine-platform
    PRIVATE Vulkan::Vulkan
    PRIVATE glfw
    PRIVATE glm::glm
    PRIVATE spdlog::spdlog
)
