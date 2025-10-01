#pragma once
#include "settings.hpp"


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
    const QueueFamilyIndices &       indices,
    const vk::raii::SwapchainKHR *   oldSwapchain = nullptr );

  // Reads a SPIR-V binary file into a vector<uint32_t>
  std::vector<uint32_t> readSpirvFile( const std::string & filePath );

  // Creates a shader module from SPIR-V code
  vk::raii::ShaderModule createShaderModule( const vk::raii::Device & device, const std::vector<uint32_t> & spirv );

  // Pipeline layout (no descriptors for this basic triangle)
  vk::raii::PipelineLayout createPipelineLayout( const vk::raii::Device & device );

  // Graphics pipeline using dynamic rendering
  vk::raii::Pipeline createGraphicsPipeline(
    const vk::raii::Device &         device,
    vk::raii::PipelineLayout const & pipelineLayout,
    vk::Extent2D                     extent,
    vk::raii::ShaderModule const &   vert,
    vk::raii::ShaderModule const &   frag,
    vk::Format                       colorFormat );

  // Command pool and buffersW
  struct CommandResources
  {
    vk::raii::CommandPool                pool = nullptr;
    std::vector<vk::raii::CommandBuffer> buffers;  // one per framebuffer
  };

  CommandResources createCommandResources( const vk::raii::Device & device, uint32_t graphicsQueueFamilyIndex, size_t count );

  void recordTriangleCommands(
    std::vector<vk::raii::CommandBuffer> const & commandBuffers,
    std::vector<vk::raii::ImageView> const &     imageViews,
    vk::Extent2D                                 extent,
    vk::raii::Pipeline const &                   pipeline );

  // Sync objects per frame-in-flight (use 2)
  struct SyncObjects
  {
    std::vector<vk::raii::Semaphore> imageAvailable;
    std::vector<vk::raii::Semaphore> renderFinished;
    std::vector<vk::raii::Fence>     inFlightFences;
  };

  SyncObjects createSyncObjects( const vk::raii::Device & device, size_t framesInFlight );

  // Draw one frame
  uint32_t drawFrame(
    const vk::raii::Device &                     device,
    const vk::raii::SwapchainKHR &               swapchain,
    vk::raii::Queue const &                      graphicsQueue,
    vk::raii::Queue const &                      presentQueue,
    std::vector<vk::raii::CommandBuffer> const & commandBuffers,
    SyncObjects &                                sync,
    size_t &                                     currentFrame );

}  // namespace core
