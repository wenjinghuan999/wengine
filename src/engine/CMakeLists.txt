add_library(engine
    engine.cpp
    ${PROJECT_SOURCE_DIR}/include/engine/engine.h
)

target_include_directories(engine
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(engine
    PRIVATE fmt::fmt
)
