cmake_minimum_required(VERSION 3.20)

set(TARGET_NAME tri-vbuf-vma)

# Set output directory for this specific subproject
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${TARGET_NAME})

add_executable(${TARGET_NAME}
    main.cpp
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        core
)

# Copy shaders folder to build output directory
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${CMAKE_BINARY_DIR}/bin/${TARGET_NAME}/shaders
    COMMENT "Copying shaders to build directory"
)

# Include directories if needed
# target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
