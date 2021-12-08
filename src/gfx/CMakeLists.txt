add_library(wengine-gfx
    gfx.cpp
    ${PROJECT_SOURCE_DIR}/include/gfx/gfx.h
)

target_include_directories(wengine-gfx
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(wengine-gfx
    PUBLIC wengine-engine
    PUBLIC wengine-platform
    
    PUBLIC glm::glm
    PRIVATE Vulkan::Vulkan
    PRIVATE glfw
    PRIVATE spdlog::spdlog
)
