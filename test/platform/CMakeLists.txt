add_executable(platform-test platform-test.cpp $<TARGET_OBJECTS:wengine-platform>)
target_link_libraries(platform-test PRIVATE wengine-platform doctest::doctest fmt::fmt)
target_include_directories(platform-test PRIVATE $<TARGET_PROPERTY:wengine-platform,INCLUDE_DIRECTORIES>)
add_test(NAME platform-test COMMAND platform-test)