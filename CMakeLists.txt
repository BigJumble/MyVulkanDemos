cmake_minimum_required(VERSION 3.20)

set(PROJECT_NAME MyEngine)
project(${PROJECT_NAME} 
    VERSION 1.0.0
    DESCRIPTION "Demo projects using Vulkan and modern C++"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Set output directories (someday make it work)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # set in subprojects
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


# -------------------
# GLFW
# -------------------
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_WAYLAND ON)
set(GLFW_BUILD_X11 ON )

add_subdirectory(src/external/glfw)

# -------------------
# Vulkan
# -------------------
find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please install the Vulkan SDK and ensure it is discoverable by CMake.")
endif()


# -------------------
# GLM (header-only)
# -------------------
add_subdirectory(src/external/glm)

# -------------------
# EnTT (header-only)
# -------------------

set(ENTT_BUILD_TESTING OFF)
set(ENTT_BUILD_DOCS OFF)
set(ENTT_BUILD_EXAMPLES OFF)
# set(ENTT_INSTALL ON)

add_subdirectory(src/external/entt)

# -------------------
# SPIRV-Reflect
# -------------------
set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)
set(SPIRV_REFLECT_BUILD_TESTS OFF)
set(SPIRV_REFLECT_INSTALL OFF)

add_subdirectory(src/external/SPIRV-Reflect)

# -------------------
# SPIRV-Headers (required by SPIRV-Tools)
# -------------------
add_subdirectory(src/external/SPIRV-Headers)

# -------------------
# SPIRV-Tools (required by shaderc)
# -------------------
set(SPIRV_SKIP_TESTS ON CACHE INTERNAL "")
set(SPIRV_SKIP_EXECUTABLES ON CACHE INTERNAL "")
set(SPIRV_WERROR OFF CACHE INTERNAL "")

add_subdirectory(src/external/SPIRV-Tools)

# -------------------
# glslang (required by shaderc)
# -------------------
set(ENABLE_GLSLANG_BINARIES OFF CACHE INTERNAL "")
set(ENABLE_SPVREMAPPER OFF CACHE INTERNAL "")
set(SKIP_GLSLANG_INSTALL ON CACHE INTERNAL "")
set(ENABLE_CTEST OFF CACHE INTERNAL "")

add_subdirectory(src/external/glslang)

# -------------------
# nlohmann/json (header-only)
# -------------------
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")

add_subdirectory(src/external/json)

# -------------------
# sol2 (header-only)
# -------------------
set(SOL2_BUILD_LUA OFF CACHE INTERNAL "")
set(SOL2_LUA_VERSION "5.4.7" CACHE STRING "")
set(SOL2_ENABLE_INSTALL OFF CACHE INTERNAL "")

add_subdirectory(src/external/sol2)

# -------------------
# shaderc
# -------------------
set(SHADERC_SKIP_INSTALL ON CACHE INTERNAL "")
set(SHADERC_SKIP_TESTS ON CACHE INTERNAL "")
set(SHADERC_SKIP_EXAMPLES ON CACHE INTERNAL "")
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE INTERNAL "")
set(SHADERC_SKIP_ASCIIDOCTOR ON CACHE INTERNAL "")

add_subdirectory(src/external/shaderc)

# -------------------
# Vulkan Memory Allocator (header-only)
# -------------------
add_subdirectory(src/external/VulkanMemoryAllocator)

# -------------------
# ImGui
# -------------------
set(IMGUI_DIR src/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    # Backend implementations
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC 
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

# -------------------
# Core library
# -------------------
add_subdirectory(src/core)

# -------------------
# Executable subprojects
# -------------------
add_subdirectory(src/subprojects)