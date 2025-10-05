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
        EnTT::EnTT
        spirv-reflect-static)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Setup Vulkan platform-specific definitions
# add wayland later
if( WIN32 )
target_compile_definitions( core PUBLIC VK_USE_PLATFORM_WIN32_KHR )
elseif( APPLE )
target_compile_definitions( core PUBLIC VK_USE_PLATFORM_METAL_EXT )
elseif( UNIX )
target_compile_definitions( core PUBLIC VK_USE_PLATFORM_XCB_KHR )
else()
message( FATAL_ERROR "Vulkan-Hpp: unhandled platform!" )
endif()