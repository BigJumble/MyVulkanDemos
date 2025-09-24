#pragma once

#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_NONE
#include "settings.hpp"

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

// int add( int a, int b );

namespace core
{
  // using Transfrom = glm::mat4x4;

  VKAPI_ATTR vk::Bool32 VKAPI_CALL debugUtilsMessengerCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT              messageTypes,
    const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData,
    void * /*pUserData*/ );

  vk::InstanceCreateInfo
    createInstanceCreateInfo( const std::string & appName, const std::string & engineName, std::vector<const char *> layers, std::vector<const char *> extensions );

  vk::DebugUtilsMessengerCreateInfoEXT createDebugUtilsMessengerCreateInfo();

  vk::raii::PhysicalDevice selectPhysicalDevice( const vk::raii::PhysicalDevices & devices );

  // contains window and sufrace
  struct DisplayBundle
  {
    DisplayBundle( vk::raii::Instance const & instance, std::string const & windowName, vk::Extent2D const & extent_ );

    ~DisplayBundle();

    // Delete copy constructor and copy assignment operator to prevent copying of DisplayBundle,
    // since it manages non-copyable resources (GLFWwindow*, vk::raii::SurfaceKHR).
    DisplayBundle( const DisplayBundle & )             = delete;
    DisplayBundle & operator=( const DisplayBundle & ) = delete;

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

    bool isComplete() const
    {
      return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX;
    }
  };

  QueueFamilyIndices findQueueFamilies( const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::SurfaceKHR & surface );

  struct DeviceBundle
  {
    vk::raii::Device   device        = nullptr;
    vk::raii::Queue    graphicsQueue = nullptr;
    vk::raii::Queue    presentQueue  = nullptr;
    vk::raii::Queue    computeQueue  = nullptr;
    QueueFamilyIndices indices;
  };

  DeviceBundle createDeviceWithQueues( const vk::raii::PhysicalDevice & physicalDevice, const QueueFamilyIndices & indices );

  struct SwapchainSupportDetails
  {
    vk::SurfaceCapabilitiesKHR        capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR>   presentModes;
  };

  struct SwapchainBundle
  {
    vk::raii::SwapchainKHR           swapchain   = nullptr;
    vk::Format                       imageFormat = vk::Format::eUndefined;
    vk::Extent2D                     extent{};
    std::vector<vk::Image>           images;      // raw images owned by swapchain
    std::vector<vk::raii::ImageView> imageViews;  // views created for each swapchain image
  };

  SwapchainSupportDetails querySwapchainSupport( const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::SurfaceKHR & surface );

  vk::SurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<vk::SurfaceFormatKHR> & availableFormats );

  vk::PresentModeKHR chooseSwapPresentMode( const std::vector<vk::PresentModeKHR> & availablePresentModes );

  vk::Extent2D chooseSwapExtent( const vk::SurfaceCapabilitiesKHR & capabilities, const vk::Extent2D & desiredExtent );

  SwapchainBundle createSwapchain(
    const vk::raii::PhysicalDevice & physicalDevice,
    const vk::raii::Device &         device,
    const vk::raii::SurfaceKHR &     surface,
    const vk::Extent2D &             desiredExtent,
    const QueueFamilyIndices &       indices );

}  // namespace core
