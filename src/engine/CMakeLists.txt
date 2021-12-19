add_library(wengine-engine
    engine.cpp
    ${PROJECT_SOURCE_DIR}/include/engine/engine.h
)

target_include_directories(wengine-engine
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(wengine-engine
    PUBLIC wengine-common
    PRIVATE fmt::fmt
)
