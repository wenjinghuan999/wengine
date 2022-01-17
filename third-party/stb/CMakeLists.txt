FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG af1a5bc352164740c1cc1354942b1c6b72eacb8a # master
)
FetchContent_MakeAvailable(stb)

add_library(third-party-stb INTERFACE)
target_include_directories(third-party-stb
    INTERFACE "${stb_SOURCE_DIR}")