# Comprehensive benchmark suite

# Only build comprehensive_benchmark if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/comprehensive_benchmark.cpp")
    add_executable(fs_benchmark
        src/comprehensive_benchmark.cpp
    )

    # Include directories for headers
    target_include_directories(fs_benchmark PRIVATE
        ${CMAKE_SOURCE_DIR}/fs/include
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/cache/include
        ${CMAKE_SOURCE_DIR}/storage/include
        ${CMAKE_SOURCE_DIR}/journal/include
        ${CMAKE_SOURCE_DIR}/threading/include
    )

    # Link necessary libraries
    target_link_libraries(fs_benchmark
        fs
        common
        cache
        storage
        journal
        threading
    )

    # Install benchmark executable
    install(TARGETS fs_benchmark
        RUNTIME DESTINATION bin
    )
endif()

# Main benchmark with live features
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark_main.cpp")
    add_executable(benchmark_main
        src/benchmark_main.cpp
    )

    target_include_directories(benchmark_main PRIVATE
        ${CMAKE_SOURCE_DIR}/fs/include
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/cache/include
        ${CMAKE_SOURCE_DIR}/storage/include
        ${CMAKE_SOURCE_DIR}/journal/include
        ${CMAKE_SOURCE_DIR}/threading/include
    )

    target_link_libraries(benchmark_main
        fs
        common
        cache
        storage
        journal
        threading
    )
endif()

# Real comparison benchmark (your implementations vs standard library)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/real_comparison_benchmark.cpp")
    add_executable(real_comparison_benchmark
        src/real_comparison_benchmark.cpp
    )

    target_include_directories(real_comparison_benchmark PRIVATE
        ${CMAKE_SOURCE_DIR}/fs/include
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/cache/include
        ${CMAKE_SOURCE_DIR}/storage/include
        ${CMAKE_SOURCE_DIR}/journal/include
        ${CMAKE_SOURCE_DIR}/threading/include
    )

    target_link_libraries(real_comparison_benchmark
        fs
        common
        cache
        storage
        journal
        threading
    )

    # Install benchmark executable
    install(TARGETS real_comparison_benchmark
        RUNTIME DESTINATION bin
    )
endif()