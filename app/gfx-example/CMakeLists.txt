add_executable(gfx-example
    gfx-example.cpp
)

target_include_directories(gfx-example
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(gfx-example
    PRIVATE platform
    PRIVATE gfx
)
