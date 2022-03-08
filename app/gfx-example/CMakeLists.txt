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

add_library(gfx-examples-interface
    INTERFACE)

target_include_directories(gfx-examples-interface
    INTERFACE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(gfx-examples-interface
    INTERFACE wengine-platform
    INTERFACE wengine-engine
    INTERFACE wengine-gfx)

add_dependencies(gfx-examples-interface
    INTERFACE wengine-shader-static
    INTERFACE wengine-resources)

add_executable(gfx-engine-example
    gfx-engine-example.cpp)

target_link_libraries(gfx-engine-example
    PRIVATE gfx-examples-interface)

add_executable(gfx-example-anisotropy
    gfx-example-anisotropy.cpp)

target_link_libraries(gfx-example-anisotropy
    PRIVATE gfx-examples-interface)

add_executable(gfx-example-sampler.cpp
    gfx-example-sampler.cpp)

target_link_libraries(gfx-example-sampler.cpp
    PRIVATE gfx-examples-interface)

add_executable(gfx-example-msaa
    gfx-example-msaa.cpp)

target_link_libraries(gfx-example-msaa
    PRIVATE gfx-examples-interface)
