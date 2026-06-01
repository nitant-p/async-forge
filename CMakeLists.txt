cmake_minimum_required(VERSION 3.20)

project(
    async_forge
    VERSION 0.1.0
    DESCRIPTION "Learning project for blocking, threaded, event-driven, and async C++ servers"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ASYNC_FORGE_BUILD_TESTS "Build async-forge tests" ON)

add_library(async_forge_core
    src/logging.cpp
)

target_include_directories(async_forge_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(async_forge_core
    PUBLIC
        cxx_std_20
)

if(MSVC)
    target_compile_options(async_forge_core PRIVATE /W4)
else()
    target_compile_options(async_forge_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(echo_server
    apps/echo_server.cpp
)

target_link_libraries(echo_server
    PRIVATE
        async_forge_core
)

add_executable(echo_client
    apps/echo_client.cpp
)

target_link_libraries(echo_client
    PRIVATE
        async_forge_core
)

if(ASYNC_FORGE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
