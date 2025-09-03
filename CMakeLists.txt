cmake_minimum_required(VERSION 3.15)

# Set CMake policies
cmake_policy(SET CMP0135 NEW)

project(MultiThreadedFS VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add subdirectories for all components
add_subdirectory(fs)
add_subdirectory(common)
add_subdirectory(cache)
add_subdirectory(storage)
add_subdirectory(journal)
add_subdirectory(threading)
add_subdirectory(test)
add_subdirectory(cli)
add_subdirectory(benchmark)

# Create threading demo executable (commented out - missing source file)
# add_executable(threading_demo threading_demo.cpp)
# target_link_libraries(threading_demo
#     threading
#     fs
#     common
#     cache
#     storage
#     journal
# )
# target_include_directories(threading_demo PRIVATE
#     threading/include
#     fs/include
#     common/include
#     cache/include
#     storage/include
#     journal/include
# )

# Enable testing
enable_testing()

# Installation rules
install(TARGETS
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)