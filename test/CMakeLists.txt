FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/onqtam/doctest.git
    GIT_TAG 2.4.6
)
FetchContent_MakeAvailable(doctest)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)
add_subdirectory(common)
add_subdirectory(gfx)
add_subdirectory(platform)