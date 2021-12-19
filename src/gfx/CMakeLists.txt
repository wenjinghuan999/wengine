add_library(wengine-gfx
    gfx.cpp
    gfx-constants.cpp
    gfx-pipeline.cpp
    shader.cpp
    surface.cpp
    render-target.cpp
    inc/gfx-private.h
    inc/gfx-constants-private.h
    inc/gfx-pipeline-private.h
    inc/shader-private.h
    inc/surface-private.h
    inc/render-target-private.h
    ${PROJECT_SOURCE_DIR}/include/gfx/gfx.h
    ${PROJECT_SOURCE_DIR}/include/gfx/gfx-constants.h
    ${PROJECT_SOURCE_DIR}/include/gfx/gfx-pipeline.h
    ${PROJECT_SOURCE_DIR}/include/gfx/shader.h
    ${PROJECT_SOURCE_DIR}/include/gfx/surface.h
    ${PROJECT_SOURCE_DIR}/include/gfx/render-target.h
)

add_dependencies(wengine-gfx wengine-shader-static wengine-shader-dynamic)

target_include_directories(wengine-gfx
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${PROJECT_SOURCE_DIR}/src
    PUBLIC ${PROJECT_SOURCE_DIR}/src/gfx/inc
)

target_link_libraries(wengine-gfx
    PUBLIC wengine-logger
    PUBLIC wengine-engine
    PUBLIC wengine-platform
    
    PUBLIC glm::glm
    PRIVATE Vulkan::Vulkan
    PRIVATE glfw
)
