cmake_minimum_required(VERSION 3.12)

set(PROJECT_NAME MyEngine)
project(${PROJECT_NAME} CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# -------------------
# GLFW
# -------------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
# set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)

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
# Demo executable
# -------------------
add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw Vulkan::Vulkan glm)

target_include_directories(${PROJECT_NAME} PRIVATE
    src/external/glm
)