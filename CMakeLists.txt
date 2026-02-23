cmake_minimum_required(VERSION 3.25)

cmake_policy(SET CMP0048 NEW)

project(BlinkBreak
    VERSION 0.1.0
    DESCRIPTION "Eye strain prevention application"
    LANGUAGES CXX
)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BLINKBREAK_BUILD_TESTS "Build unit tests" ON)
option(BLINKBREAK_BUILD_DOCS "Build documentation" OFF)



# Include FetchContent for dependencies
include(FetchContent)

# Fetch Google Test
if(BLINKBREAK_BUILD_TESTS)
    if(EXISTS "${CMAKE_BINARY_DIR}/_deps/googletest-src/CMakeLists.txt")
        set(FETCHCONTENT_SOURCE_DIR_GOOGLETEST "${CMAKE_BINARY_DIR}/_deps/googletest-src")
    endif()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    include(GoogleTest)
endif()

# Fetch rapidjson
if(EXISTS "${CMAKE_BINARY_DIR}/_deps/rapidjson-src/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_RAPIDJSON "${CMAKE_BINARY_DIR}/_deps/rapidjson-src")
endif()
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG v1.1.0
)
FetchContent_MakeAvailable(rapidjson)
if(NOT TARGET rapidjson::rapidjson)
    add_library(rapidjson::rapidjson INTERFACE IMPORTED)
    set_target_properties(rapidjson::rapidjson PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${rapidjson_SOURCE_DIR}/include"
    )
endif()

# Fetch spdlog
if(EXISTS "${CMAKE_BINARY_DIR}/_deps/spdlog-src/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_SPDLOG "${CMAKE_BINARY_DIR}/_deps/spdlog-src")
endif()
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(spdlog)
target_compile_definitions(spdlog PRIVATE _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)

# Fetch Slint
set(SLINT_FEATURE_INTERPRETER OFF CACHE BOOL "" FORCE)
set(ENV{RUSTFLAGS} "$ENV{RUSTFLAGS} -Awarnings")
FetchContent_Declare(
    slint
    GIT_REPOSITORY https://github.com/slint-ui/slint.git
    GIT_TAG v1.15.1
    SOURCE_SUBDIR api/cpp
)
FetchContent_MakeAvailable(slint)

# Add subdirectories
add_subdirectory(src)

if(BLINKBREAK_BUILD_TESTS)
    add_subdirectory(tests)
endif()
