add_executable(gfx-test gfx-test.cpp $<TARGET_OBJECTS:gfx>)
target_link_libraries(gfx-test PRIVATE gfx doctest::doctest fmt::fmt)
target_include_directories(gfx-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME gfx-test COMMAND gfx-test)