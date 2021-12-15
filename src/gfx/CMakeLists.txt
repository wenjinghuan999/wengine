add_library(wengine-gfx
    gfx.cpp
    ${PROJECT_SOURCE_DIR}/include/gfx/gfx.h
)

add_dependencies(wengine-gfx wengine-shader-static wengine-shader-dynamic)

target_include_directories(wengine-gfx
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${PROJECT_SOURCE_DIR}/src/inc
)

target_link_libraries(wengine-gfx
    PUBLIC wengine-logger
    PUBLIC wengine-engine
    PUBLIC wengine-platform
    
    PUBLIC glm::glm
    PRIVATE Vulkan::Vulkan
    PRIVATE glfw
)
