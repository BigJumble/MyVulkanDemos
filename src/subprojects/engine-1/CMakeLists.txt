cmake_minimum_required(VERSION 3.20)

set(TARGET_NAME engine-1)

# Set output directory for this specific subproject
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${TARGET_NAME})

add_executable(${TARGET_NAME}
    main.cpp
    setup.cpp
)


target_link_libraries(${TARGET_NAME}
    PRIVATE
        glfw
        Vulkan::Vulkan
        glm
        imgui
        EnTT::EnTT
        spirv-reflect-static
        nlohmann_json::nlohmann_json
        sol2::sol2
        shaderc
        VulkanMemoryAllocator
)

# Setup Vulkan platform-specific definitions
# add wayland later
if( WIN32 )
target_compile_definitions( ${TARGET_NAME} PUBLIC VK_USE_PLATFORM_WIN32_KHR )
elseif( APPLE )
target_compile_definitions( ${TARGET_NAME} PUBLIC VK_USE_PLATFORM_METAL_EXT )
elseif( UNIX )
# target_compile_definitions( ${TARGET_NAME} PUBLIC VK_USE_PLATFORM_XCB_KHR )
target_compile_definitions( ${TARGET_NAME} PUBLIC VK_USE_PLATFORM_WAYLAND_KHR )
else()
message( FATAL_ERROR "Vulkan-Hpp: unhandled platform!" )
endif()

# Copy shaders folder to build output directory
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${CMAKE_BINARY_DIR}/bin/${TARGET_NAME}/shaders
    COMMENT "Copying shaders to build directory"
)

# Include directories if needed
# target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
