cmake_minimum_required(VERSION 4.1.1)

set(TARGET_NAME basic)

add_executable(${TARGET_NAME}
    main.cpp
)

# Link against available libraries
# core is your local library; glfw, Vulkan::Vulkan, glm, imgui, and EnTT are provided/added at top-level

target_link_libraries(${TARGET_NAME}
    PRIVATE
        core
        # glfw
        # Vulkan::Vulkan
        # glm
        # imgui
        # EnTT::EnTT
)

# Include directories if needed
# target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
