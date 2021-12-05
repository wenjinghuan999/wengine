add_library(gfx
    gfx.cpp
    ${PROJECT_SOURCE_DIR}/include/gfx/gfx.h
)

target_include_directories(gfx
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(gfx
    PUBLIC glm::glm
    PRIVATE Vulkan::Vulkan
    PRIVATE glfw
    PRIVATE spdlog::spdlog
)
