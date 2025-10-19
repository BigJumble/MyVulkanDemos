#pragma once
#include "settings.hpp"

namespace core
{

  [[nodiscard]] vk::raii::Instance createInstance( vk::raii::Context & context, const std::string & appName, const std::string & engineName );

  [[nodiscard]] vk::raii::PhysicalDevice selectPhysicalDevice( const vk::raii::PhysicalDevices & devices );

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
    std::optional<uint32_t> graphicsFamily;  // For ray tracing + graphics + compute post processing + presentation
    std::optional<uint32_t> presentFamily;   // no support for niche hardware
    std::optional<uint32_t> computeFamily;   // For async compute physics simulation

    bool isComplete() const
    {
      return graphicsFamily.has_value() &&
             presentFamily.has_value() &&
             computeFamily.has_value();
    }
  };

  [[nodiscard]] QueueFamilyIndices findQueueFamilies( const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::SurfaceKHR & surface );

  struct DeviceBundle
  {
    vk::raii::Device device{ nullptr };
    vk::raii::Queue  graphicsQueue{ nullptr };
    vk::raii::Queue  presentQueue{ nullptr };
    vk::raii::Queue  computeQueue{ nullptr };
    QueueFamilyIndices indices;
  };

  [[nodiscard]] DeviceBundle createDeviceWithQueues( const vk::raii::PhysicalDevice & physicalDevice, const QueueFamilyIndices & indices );

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

  [[nodiscard]] SwapchainSupportDetails querySwapchainSupport( const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::SurfaceKHR & surface );

  [[nodiscard]] vk::SurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<vk::SurfaceFormatKHR> & availableFormats );

  [[nodiscard]] vk::PresentModeKHR chooseSwapPresentMode( const std::vector<vk::PresentModeKHR> & availablePresentModes );

  [[nodiscard]] vk::Extent2D chooseSwapExtent( const vk::SurfaceCapabilitiesKHR & capabilities, const vk::Extent2D & desiredExtent );

  [[nodiscard]] SwapchainBundle createSwapchain(
    const vk::raii::PhysicalDevice & physicalDevice,
    const vk::raii::Device &         device,
    const vk::raii::SurfaceKHR &     surface,
    const vk::Extent2D &             desiredExtent,
    const QueueFamilyIndices &       indices,
    const vk::raii::SwapchainKHR *   oldSwapchain = nullptr );

  // Reads a SPIR-V binary file into a vector<uint32_t>
  [[nodiscard]] std::vector<uint32_t> readSpirvFile( const std::string & filePath );

  // Creates a shader module from SPIR-V code
  [[nodiscard]] vk::raii::ShaderModule createShaderModule( const vk::raii::Device & device, const std::vector<uint32_t> & spirv );

}  // namespace core

