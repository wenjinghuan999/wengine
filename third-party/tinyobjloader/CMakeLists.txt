FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG v2.0.0rc9
)
FetchContent_MakeAvailable(tinyobjloader)

add_library(third-party-tinyobjloader INTERFACE)
target_include_directories(third-party-tinyobjloader
    INTERFACE "${tinyobjloader_SOURCE_DIR}")
