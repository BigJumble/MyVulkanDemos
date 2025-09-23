# Base library
file(GLOB BASE_SOURCES "*.cpp")
add_library(core STATIC
    ${BASE_SOURCES}
)
target_link_libraries(
        core PUBLIC 
        glfw
        Vulkan::Vulkan
        glm
        imgui
        EnTT::EnTT)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
