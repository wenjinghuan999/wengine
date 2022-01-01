add_executable(gfx-test gfx-test.cpp $<TARGET_OBJECTS:wengine-gfx>)
target_link_libraries(gfx-test PRIVATE wengine-gfx doctest::doctest fmt::fmt)
target_include_directories(gfx-test PRIVATE $<TARGET_PROPERTY:wengine-gfx,INCLUDE_DIRECTORIES>)
add_test(NAME gfx-test COMMAND gfx-test)