cmake_minimum_required(VERSION 3.21)
project(wengine
    DESCRIPTION "WEngine, a toy engine."
    VERSION 0.0.1
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/binaries)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${PROJECT_SOURCE_DIR}/extern)

# Adds Vulkan::Vulkan
find_package(Vulkan REQUIRED)

# Adds fmt::fmt
FetchContent_Declare(
    fmtlib
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG ac1b5f3da532fc9dca349c1bc06de65178f44eb3 # 8.0.1-master
)
FetchContent_MakeAvailable(fmtlib)

# Adds glm::glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# Adds spdlog::spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.9.2
)
set(SPDLOG_FMT_EXTERNAL ON)
set(SPDLOG_NO_EXCEPTIONS ON)
FetchContent_MakeAvailable(spdlog)

# Adds glfw
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.5
)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
FetchContent_MakeAvailable(glfw)
if (MSVC)
    target_compile_options(glfw PUBLIC /W3)
else ()
    target_compile_options(glfw PUBLIC -w)
endif ()

add_subdirectory(app)
add_subdirectory(src)
add_subdirectory(third-party)
add_subdirectory(resources)

include(CTest)
enable_testing()
add_subdirectory(test)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
