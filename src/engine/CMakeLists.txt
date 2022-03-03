add_library(wengine-engine
    material.cpp
    mesh.cpp
    mesh-component.cpp
    scene-renderer.cpp
    texture.cpp
    ${PROJECT_SOURCE_DIR}/include/engine/material.h
    ${PROJECT_SOURCE_DIR}/include/engine/mesh.h
    ${PROJECT_SOURCE_DIR}/include/engine/mesh-component.h
    ${PROJECT_SOURCE_DIR}/include/engine/scene-renderer.h
    ${PROJECT_SOURCE_DIR}/include/engine/texture.h)

target_include_directories(wengine-engine
    PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(wengine-engine
    PUBLIC wengine-common
    PUBLIC wengine-logger
    PUBLIC wengine-gfx
    PRIVATE third-party-tinyobjloader)
