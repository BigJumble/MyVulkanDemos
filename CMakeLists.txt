cmake_minimum_required(VERSION 3.12)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(PROJECT_NAME MyEngine)
project(${PROJECT_NAME} CXX)

# Set C++ compiler to GCC 14
set(CMAKE_CXX_COMPILER g++-14)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(src/external)
include_directories(src/external/glfw)
include_directories(src/external/glm)


OPTION(USE_D2D_WSI "Build the project using Direct to Display swapchain" OFF)
OPTION(USE_DIRECTFB_WSI "Build the project using DirectFB swapchain" OFF)
OPTION(USE_WAYLAND_WSI "Build the project using Wayland swapchain" OFF)
OPTION(USE_HEADLESS "Build the project using headless extension swapchain" OFF)
OPTION(USE_RELATIVE_ASSET_PATH "Load assets (shaders, models, textures) from a fixed path relative to the binar" OFF)
OPTION(FORCE_VALIDATION "Forces validation on for all samples at compile time (prefer using the -v / --validation command line arguments)" OFF)


# Platform detection
IF(UNIX AND NOT APPLE)
	set(LINUX TRUE)
ENDIF()

# Find Vulkan
message(STATUS "Using module to find Vulkan")
find_package(Vulkan REQUIRED)

# Platform-specific Vulkan setup
IF(WIN32)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSEIF(LINUX)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
	find_package(Threads REQUIRED)
	find_package(XCB REQUIRED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
ELSEIF(APPLE)
	IF (NOT Vulkan_FOUND)
		set(CMAKE_FIND_FRAMEWORK NEVER)
		find_library(Vulkan_LIBRARY NAMES MoltenVK HINTS "$ENV{VULKAN_SDK}/lib" "$ENV{VULKAN_SDK}/dylib/macOS" REQUIRED)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE(STATUS "Using MoltenVK Vulkan Portability library")
		ENDIF()
	ENDIF()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_METAL_EXT")
ENDIF()

IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS "Found Vulkan library: ${Vulkan_LIBRARY}")
ENDIF()

# Set preprocessor defines
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")

# Clang specific stuff
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch-enum")
endif()

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

add_executable(${PROJECT_NAME} src/main.cpp)

# Link Vulkan and platform-specific libraries
IF(WIN32)
	target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARY} glfw)
ELSEIF(APPLE)
	target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARY} glfw "-framework AppKit" "-framework QuartzCore")
ELSE(WIN32)
	target_link_libraries(${PROJECT_NAME} ${XCB_LIBRARIES} ${Vulkan_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} glfw)
ENDIF(WIN32)

# add_subdirectory(src/external)
set( GLFW_BUILD_EXAMPLES OFF )  # Don't build GLFW examples
set( GLFW_BUILD_TESTS OFF )     # Don't build GLFW tests
set( GLFW_BUILD_WAYLAND OFF )   # Disable Wayland support to avoid missing dependencies
add_subdirectory(src/external/glfw)
