cmake_minimum_required(VERSION 3.20)

set(TARGET_NAME shadertoy)

add_executable(${TARGET_NAME}
    main.cpp
    rendering.cpp
    swapchain_utils.cpp
    ui.cpp
)


# Link against available libraries
# core is your local library; glfw, Vulkan::Vulkan, glm, imgui, and EnTT are provided/added at top-level

target_link_libraries(${TARGET_NAME}
    PRIVATE
        core
)

# Include directories if needed
# target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
