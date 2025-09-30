#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_NONE


#include <GLFW/glfw3.h>
#include <chrono>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <print>
#include <string>
#include <thread>
#include <vector>

#if defined( DEBUG ) || !defined( NDEBUG )
#  define isDebug( code ) code
#else
#  define isDebug( code )
#endif

namespace core
{
  // User defined presentation mode
  inline constexpr vk::PresentModeKHR preferedPresentationMode = vk::PresentModeKHR::eFifoRelaxed;

  // clang-format off
  // Debug message severity flags
  inline constexpr vk::DebugUtilsMessageSeverityFlagsEXT DebugMessageSeverity =
    // vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
    // vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

  // clang-format off
  // Debug message type flags
  inline constexpr vk::DebugUtilsMessageTypeFlagsEXT DebugMessageType =
    // vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding |
    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

  // List of required device extensions
  inline const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    // Add more device extensions here if needed
  };

  // clang-format off
  // List of required instance extensions
  inline const std::vector<const char *> InstanceExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
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
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
  };
  // clang-format on
}  // namespace core
