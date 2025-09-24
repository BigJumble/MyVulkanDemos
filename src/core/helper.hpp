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
#include "settings.hpp"




// int add( int a, int b );

namespace core
{
  // using Transfrom = glm::mat4x4;

  VKAPI_ATTR vk::Bool32 VKAPI_CALL debugUtilsMessengerCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT messageTypes, const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData, void * /*pUserData*/ );

  vk::InstanceCreateInfo createInstanceCreateInfo( const std::string & appName, const std::string & engineName, std::vector<const char *> layers, std::vector<const char *> extensions );

  vk::DebugUtilsMessengerCreateInfoEXT createDebugUtilsMessengerCreateInfo();

  vk::raii::PhysicalDevice selectPhysicalDevice( const vk::raii::PhysicalDevices & devices );
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

  struct QueueFamilyIndices
  {
    uint32_t graphicsFamily = UINT32_MAX;
    uint32_t presentFamily  = UINT32_MAX;
    uint32_t computeFamily  = UINT32_MAX;

    bool isComplete() const { return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX; }
  };

  QueueFamilyIndices findQueueFamilies( const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::SurfaceKHR & surface );

  struct DeviceBundle
  {
    vk::raii::Device device = nullptr;
    vk::raii::Queue  graphicsQueue = nullptr;
    vk::raii::Queue  presentQueue  = nullptr;
    vk::raii::Queue  computeQueue  = nullptr;
    QueueFamilyIndices indices;
  };

  DeviceBundle createDeviceWithQueues( const vk::raii::PhysicalDevice & physicalDevice, const QueueFamilyIndices & indices );

}  // namespace core
