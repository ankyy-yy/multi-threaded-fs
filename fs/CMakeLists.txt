add_library(fs
    src/filesystem.cpp
    src/compression.cpp
    src/backup_manager.cpp
)

target_include_directories(fs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(fs
    PUBLIC
        common
        storage
        cache
        journal
)

# Install headers
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
) 