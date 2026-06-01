add_executable(async_forge_tests
    test_smoke.cpp
)

target_link_libraries(async_forge_tests
    PRIVATE
        async_forge_core
)

add_test(
    NAME async_forge_smoke
    COMMAND async_forge_tests
)
