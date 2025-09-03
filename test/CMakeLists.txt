cmake_minimum_required(VERSION 3.15)

enable_testing()

# Only build unit tests if GTest is available
find_package(GTest QUIET)
if(GTest_FOUND)
    add_executable(unit_tests
        test_filesystem.cpp
        test_cache.cpp
        test_lifo_cache.cpp
    )

    target_link_libraries(unit_tests
        PRIVATE
            fs
            cache
            storage
            journal
            common
            gtest
            gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(unit_tests)
endif()

# Integration test (independent of GTest)
add_executable(integration_test
    src/integration_test.cpp
)

target_include_directories(integration_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(integration_test
    PRIVATE
        fs
        cache
        storage
        common
        journal
)

# Add the test to CTest
add_test(NAME IntegrationTest COMMAND integration_test) 