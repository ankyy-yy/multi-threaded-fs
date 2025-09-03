cmake_minimum_required(VERSION 3.15)

# CLI executable
add_executable(mtfs_cli
    src/main.cpp
)

target_include_directories(mtfs_cli
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/fs/include
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/cache/include
        ${CMAKE_SOURCE_DIR}/storage/include
        ${CMAKE_SOURCE_DIR}/journal/include
        ${CMAKE_SOURCE_DIR}/threading/include
)

target_link_libraries(mtfs_cli
    PRIVATE
        fs
        cache
        storage
        common
        journal
        threading
) 