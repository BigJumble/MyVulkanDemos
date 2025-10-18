cmake_minimum_required(VERSION 3.20)

set(TARGET_NAME camera-instancing)

add_executable(${TARGET_NAME}
    main.cpp
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        core
)

# Include directories if needed
# target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
