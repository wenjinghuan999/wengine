add_executable(wengine-gfx-engine-test gfx-engine-test.cpp $<TARGET_OBJECTS:wengine-engine>)
target_link_libraries(wengine-gfx-engine-test PRIVATE wengine-engine doctest::doctest fmt::fmt)
target_include_directories(wengine-gfx-engine-test PRIVATE $<TARGET_PROPERTY:wengine-engine,INCLUDE_DIRECTORIES>)
add_test(NAME wengine-gfx-engine-test COMMAND wengine-gfx-engine-test)