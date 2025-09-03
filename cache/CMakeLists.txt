cmake_minimum_required(VERSION 3.15)

# Cache library
add_library(cache INTERFACE)

target_include_directories(cache
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cache
    INTERFACE
        common
)

# Install rules
install(TARGETS cache
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*"
) 