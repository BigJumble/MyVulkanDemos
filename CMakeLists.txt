cmake_minimum_required(VERSION 3.12)

set(PROJECT_NAME MyEngine)

# Set C++ compiler to GCC 14
set(CMAKE_CXX_COMPILER g++-14)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type to Debug by default if not specified
# if(NOT CMAKE_BUILD_TYPE)
#   set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
# endif()

project(${PROJECT_NAME} CXX)

include_directories(external)

add_executable(${PROJECT_NAME} src/main.cpp)

# Add debug symbols if in Debug mode
# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#   target_compile_options(${PROJECT_NAME} PRIVATE -g)
# endif()

message(STATUS "Using module to find Vulkan")
find_package(Vulkan)