add_executable(wengine-platform-test platform-test.cpp $<TARGET_OBJECTS:wengine-platform>)
target_link_libraries(wengine-platform-test PRIVATE wengine-platform doctest::doctest fmt::fmt)
target_include_directories(wengine-platform-test PRIVATE $<TARGET_PROPERTY:wengine-platform,INCLUDE_DIRECTORIES>)
add_test(NAME wengine-platform-test COMMAND wengine-platform-test)