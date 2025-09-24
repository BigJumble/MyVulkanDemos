#pragma once

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
#define isDebug(code) code
#else
#define isDebug(code)
#endif

// int add( int a, int b );

namespace core
{
  // using Transfrom = glm::mat4x4;

  VKAPI_ATTR vk::Bool32 VKAPI_CALL debugUtilsMessengerCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT messageTypes, const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData, void * /*pUserData*/ );

  vk::InstanceCreateInfo createInstanceCreateInfo( const std::string & appName, const std::string & engineName, std::vector<const char *> layers, std::vector<const char *> extensions );

  vk::DebugUtilsMessengerCreateInfoEXT createDebugUtilsMessengerCreateInfo();

  vk::raii::PhysicalDevice selectPhysicalDevice( const vk::raii::PhysicalDevices & devices );

  std::vector<const char *> getInstanceExtensions();

  //contains window and sufrace
  struct SurfaceData
  {
    SurfaceData( vk::raii::Instance const & instance, std::string const & windowName, vk::Extent2D const & extent_ );

    ~SurfaceData();

    // Delete copy constructor and copy assignment operator to prevent copying of SurfaceData,
    // since it manages non-copyable resources (GLFWwindow*, vk::raii::SurfaceKHR).
    SurfaceData( const SurfaceData & )             = delete;
    SurfaceData & operator=( const SurfaceData & ) = delete;

    vk::Extent2D         extent;
    GLFWwindow *         window = nullptr;
    std::string          name;
    vk::raii::SurfaceKHR surface = nullptr;
  };

}  // namespace core
