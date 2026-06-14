add_executable(async_forge_tests
    test_smoke.cpp
)

target_link_libraries(async_forge_tests
    PRIVATE
        async_forge_core
)

add_executable(socket_utils_tests
    test_socket_utils.cpp
)

target_link_libraries(socket_utils_tests
    PRIVATE
        async_forge_socket_utils
)

add_test(
    NAME async_forge_smoke
    COMMAND async_forge_tests
)

add_test(
    NAME socket_utils_framing
    COMMAND socket_utils_tests
)

add_executable(server_client_tests
    test_server_client.cpp
)

target_link_libraries(server_client_tests
    PRIVATE
        async_forge_socket_utils
)

add_test(
    NAME server_client_integration
    COMMAND server_client_tests $<TARGET_FILE:echo_server>
)
