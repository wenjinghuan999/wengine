FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
    GIT_TAG v3.10.5)
FetchContent_MakeAvailable(json)

add_library(third-party-json INTERFACE)
target_include_directories(third-party-json INTERFACE
    "${json_SOURCE_DIR}/single_include")