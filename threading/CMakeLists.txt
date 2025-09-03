# Threading module for multi-threaded file system operations

# Only build if source files exist
set(THREADING_SOURCES)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/thread_pool.cpp")
    list(APPEND THREADING_SOURCES src/thread_pool.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/async_file_ops.cpp")
    list(APPEND THREADING_SOURCES src/async_file_ops.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/parallel_backup.cpp")
    list(APPEND THREADING_SOURCES src/parallel_backup.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/concurrent_cache.cpp")
    list(APPEND THREADING_SOURCES src/concurrent_cache.cpp)
endif()

# Only create library if we have source files
if(THREADING_SOURCES)
    add_library(threading ${THREADING_SOURCES})

    # Include directories
    target_include_directories(threading PUBLIC
        include
        ${CMAKE_SOURCE_DIR}/cache/include
        ${CMAKE_SOURCE_DIR}/fs/include
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/storage/include
    )

    # Link with required libraries
    target_link_libraries(threading
        cache
        common
        Threads::Threads
    )

    # Enable C++17 features
    target_compile_features(threading PUBLIC cxx_std_17)

    # Platform-specific threading support
    find_package(Threads REQUIRED)

    # Install the library
    install(TARGETS threading
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )

    # Install headers
    install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.tpp"
    )
else()
    message(STATUS "Threading module: No source files found, creating header-only library")
    add_library(threading INTERFACE)
    
    target_include_directories(threading INTERFACE
        include
        ${CMAKE_SOURCE_DIR}/cache/include
        ${CMAKE_SOURCE_DIR}/fs/include
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/storage/include
    )
    
    find_package(Threads REQUIRED)
    target_link_libraries(threading INTERFACE Threads::Threads)
    target_compile_features(threading INTERFACE cxx_std_17)
endif()
