#pragma once
// #include <vulkan/vulkan_core.h>
// #define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
// #include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>
// #include <vulkan/vulkan_handles.hpp>
// #include <vulkan/vulkan_structs.hpp>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
// #include <chrono>
#include <entt/entt.hpp>
// #include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// #include <iostream>
// #include <print>
// #include <string>
// #include <thread>
#include <vector>

#if defined( DEBUG ) || !defined( NDEBUG )
#  define isDebug( code ) code
#else
#  define isDebug( code )
#endif

namespace core
{
  // User defined presentation mode
  inline constexpr vk::PresentModeKHR preferedPresentationMode = vk::PresentModeKHR::eFifo;

  // clang-format off
  // Debug message severity flags
  // inline constexpr vk::DebugUtilsMessageSeverityFlagsEXT DebugMessageSeverity =
  //   // vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
  //   // vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
  //   vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
  //   vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

  // // clang-format off
  // // Debug message type flags
  // inline constexpr vk::DebugUtilsMessageTypeFlagsEXT DebugMessageType =
  //   // vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding |
  //   vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
  //   vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
  //   vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

  // List of required device extensions
  /*
  inline const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    // VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,  // Enables fences with presentation
    // VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, // included by default in vk 1.3+
    // VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,  // Features enabled in bootstrap.cpp
    // VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, // included by default in vk 1.3+
    // VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, // included by default in vk 1.2+
    // VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,// // included by default in vk 1.2+ todo
    // VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,  // included by default in vk 1.2+
    VK_KHR_MAINTENANCE_7_EXTENSION_NAME, 
    VK_KHR_MAINTENANCE_8_EXTENSION_NAME, 
    // VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME, //todo // included by default in vk 1.4+
    // VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,  // included by default in vk 1.4+
    // VK_EXT_SHADER_OBJECT_EXTENSION_NAME  // Features enabled in bootstrap.cpp
    
    // Ray tracing extensions
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_RAY_QUERY_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,

    // Add more device extensions here if needed
  };
  */
  // clang-format off
  // List of required instance extensions
  inline const std::vector<const char *> InstanceExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME, // sometime in the future, it will be included in the core spec
    VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
    VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_METAL_EXT )
    VK_EXT_METAL_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_VI_NN )
    VK_NN_VI_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_WAYLAND_KHR )
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_WIN32_KHR )
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_XCB_KHR )
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_XLIB_KHR )
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_XLIB_XRANDR_EXT )
    VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME,
#endif
#if defined( DEBUG ) || !defined( NDEBUG )
    // VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
  };
  // clang-format on
}  // namespace core
