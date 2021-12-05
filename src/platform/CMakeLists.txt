add_library(platform
platform.cpp
    ${PROJECT_SOURCE_DIR}/include/platform/platform.h
)

target_include_directories(platform
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(platform
    PRIVATE Vulkan::Vulkan
    PRIVATE glfw
    PRIVATE glm::glm
    PRIVATE spdlog::spdlog
)
