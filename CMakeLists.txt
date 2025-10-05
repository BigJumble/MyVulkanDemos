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


# -------------------
# GLFW
# -------------------
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_WAYLAND OFF)
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

# set(ENTT_BUILD_TESTING OFF)
# set(ENTT_BUILD_DOCS OFF)
# set(ENTT_BUILD_EXAMPLES OFF)
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
# Core library
# -------------------
add_subdirectory(src/core)

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
# Executable subprojects
# -------------------
add_subdirectory(src/subprojects)