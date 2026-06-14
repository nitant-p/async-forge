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
option(ASYNC_FORGE_ENABLE_COVERAGE "Build with coverage instrumentation" OFF)

if(ASYNC_FORGE_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O0 -g --coverage)
        add_link_options(--coverage)
    else()
        message(WARNING "Coverage instrumentation is only configured for GNU/Clang compilers")
    endif()
endif()

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

add_library(async_forge_socket_utils
    apps/SocketUtils.cpp
)

target_include_directories(async_forge_socket_utils
    PUBLIC
        ${PROJECT_SOURCE_DIR}/apps
)

target_compile_features(async_forge_socket_utils
    PUBLIC
        cxx_std_20
)

if(MSVC)
    target_compile_options(async_forge_socket_utils PRIVATE /W4)
else()
    target_compile_options(async_forge_socket_utils PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(echo_server
    apps/server.cpp
)

target_link_libraries(echo_server
    PRIVATE
        async_forge_core
        async_forge_socket_utils
)

add_executable(echo_client
    apps/client.cpp
)

target_link_libraries(echo_client
    PRIVATE
        async_forge_core
        async_forge_socket_utils
)

if(ASYNC_FORGE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
