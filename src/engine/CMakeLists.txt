add_library(wengine-engine
)

target_include_directories(wengine-engine
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(wengine-engine
    PUBLIC wengine-common
    PUBLIC wengine-logger
    PUBLIC wengine-gfx
)
