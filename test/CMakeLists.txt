FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/onqtam/doctest.git
    GIT_TAG        2.4.6
)
FetchContent_MakeAvailable(doctest)

add_subdirectory(gfx)