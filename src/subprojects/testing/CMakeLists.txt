cmake_minimum_required(VERSION 4.1.1)

set(TARGET_NAME testing)

add_executable(${TARGET_NAME}
    main2.cpp
)

# Link against available libraries

target_link_libraries(${TARGET_NAME}
    PRIVATE
        core
        glfw
        Vulkan::Vulkan
        glm
        imgui
        EnTT::EnTT
)
