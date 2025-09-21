cmake_minimum_required(VERSION 3.20)

set(PROJECT_NAME MyEngine)
project(${PROJECT_NAME} 
    VERSION 1.0.0
    DESCRIPTION "A modern C++ graphics engine with Vulkan and GLFW"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


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

# -------------------
# GLM (header-only)
# -------------------
add_subdirectory(src/external/glm)

# -------------------
# Base library
# -------------------
add_subdirectory(src/base)

# -------------------
# Executable
# -------------------
add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw Vulkan::Vulkan glm base)

target_include_directories(${PROJECT_NAME} PRIVATE
    src/external/glm
)