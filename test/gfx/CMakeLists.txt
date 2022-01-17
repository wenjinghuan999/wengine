add_executable(wengine-gfx-test gfx-test.cpp $<TARGET_OBJECTS:wengine-gfx>)
target_link_libraries(wengine-gfx-test PRIVATE wengine-gfx doctest::doctest fmt::fmt)
target_include_directories(wengine-gfx-test PRIVATE $<TARGET_PROPERTY:wengine-gfx,INCLUDE_DIRECTORIES>)
add_test(NAME wengine-gfx-test COMMAND wengine-gfx-test)