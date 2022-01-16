add_executable(gfx-example
    gfx-example.cpp)

target_include_directories(gfx-example
    PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(gfx-example
    PRIVATE wengine-platform
    PRIVATE wengine-gfx)

add_dependencies(gfx-example
    wengine-shader-static
    wengine-resources)

add_executable(gfx-engine-example
    gfx-engine-example.cpp)

target_include_directories(gfx-engine-example
    PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(gfx-engine-example
    PRIVATE wengine-platform
    PRIVATE wengine-engine
    PRIVATE wengine-gfx)

add_dependencies(gfx-engine-example
    wengine-shader-static
    wengine-resources)
