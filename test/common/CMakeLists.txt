add_executable(wengine-common-test common-test.cpp $<TARGET_OBJECTS:wengine-common>)
target_link_libraries(wengine-common-test PRIVATE wengine-common doctest::doctest fmt::fmt)
target_include_directories(wengine-common-test PRIVATE $<TARGET_PROPERTY:wengine-common,INCLUDE_DIRECTORIES>)
add_test(NAME wengine-common-test COMMAND wengine-common-test)