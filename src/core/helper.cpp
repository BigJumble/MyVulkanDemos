#include "helper.hpp"

#include "settings.hpp"

// #include <vulkan/vulkan_raii.hpp>`

namespace core
{
  VKAPI_ATTR vk::Bool32 VKAPI_CALL debugUtilsMessengerCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT              messageTypes,
    const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData,
    void * /*pUserData*/ )
  {
    std::println( "validation layer (severity: {}): {}", vk::to_string( messageSeverity ), pCallbackData->pMessage );
    return vk::False;
  }

  vk::InstanceCreateInfo
    createInstanceCreateInfo( const std::string & appName, const std::string & engineName, std::vector<const char *> layers, std::vector<const char *> extensions )
  {
    vk::ApplicationInfo applicationInfo(
      appName.c_str(),
      1,  // application version
      engineName.c_str(),
      1,  // engine version
      VK_API_VERSION_1_3 );

    vk::InstanceCreateInfo instanceCreateInfo( {}, &applicationInfo, layers, extensions );

    return instanceCreateInfo;
  }

  // Helper function to create a DebugUtilsMessengerCreateInfoEXT with default settings and callback
  vk::DebugUtilsMessengerCreateInfoEXT createDebugUtilsMessengerCreateInfo()
  {
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {};

    debugUtilsMessengerCreateInfo.messageSeverity = core::DebugMessageSeverity;

    debugUtilsMessengerCreateInfo.messageType = core::DebugMessageType;

    debugUtilsMessengerCreateInfo.pfnUserCallback = core::debugUtilsMessengerCallback;
    return debugUtilsMessengerCreateInfo;
  }

  // Selects the best physical device, prioritizing discrete GPUs.
  // Returns the index of the selected device in the input vector, or -1 if none found.
  // Selects the best physical device, prioritizing discrete GPUs.
  // Returns a vk::raii::PhysicalDevice, or a default-constructed (invalid) one if none found.
  vk::raii::PhysicalDevice selectPhysicalDevice( const vk::raii::PhysicalDevices & devices )
  {
    if ( devices.empty() )
    {
      throw std::runtime_error( "No Vulkan physical devices found." );
    }

    // First, try to find a discrete GPU
    for ( const auto & device : devices )
    {
      vk::PhysicalDeviceProperties props = device.getProperties();
      if ( props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
      {
        std::println( "device selected {}", (std::string)device.getProperties().deviceName );

        return device;
      }
    }
    std::println( "device selected {}", (std::string)devices.front().getProperties().deviceName );
    // If no discrete GPU, just return the first device
    return devices.front();
  }

  DisplayBundle::DisplayBundle( vk::raii::Instance const & instance, std::string const & windowName, vk::Extent2D const & extent_ )
    : extent( extent_ ), window( nullptr ), name( windowName ), surface( nullptr )
  {
    if ( !glfwInit() )
    {
      throw std::runtime_error( "Failed to initialize GLFW!" );
    }
    glfwSetErrorCallback( []( int error, const char * msg ) { std::cerr << "glfw: " << "(" << error << ") " << msg << std::endl; } );

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    window = glfwCreateWindow( extent.width, extent.height, windowName.c_str(), nullptr, nullptr );
    if ( !window )
    {
      glfwTerminate();
      throw std::runtime_error( "Failed to create GLFW window!" );
    }

    VkSurfaceKHR _surface;
    VkResult     err = glfwCreateWindowSurface( *instance, window, nullptr, &_surface );
    if ( err != VK_SUCCESS )
    {
      glfwDestroyWindow( window );
      glfwTerminate();
      throw std::runtime_error( "Failed to create window surface!" );
    }
    surface = vk::raii::SurfaceKHR( instance, _surface );
  }

  DisplayBundle::~DisplayBundle()
  {
    if ( window )
    {
      glfwDestroyWindow( window );
      window = nullptr;
    }
    glfwTerminate();
  }

  QueueFamilyIndices findQueueFamilies( const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::SurfaceKHR & surface )
  {
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    for ( uint32_t i = 0; i < queueFamilyProperties.size(); ++i )
    {
      const auto & props = queueFamilyProperties[i];

      if ( ( props.queueFlags & vk::QueueFlagBits::eGraphics ) && indices.graphicsFamily == UINT32_MAX )
      {
        indices.graphicsFamily = i;
      }

      if ( physicalDevice.getSurfaceSupportKHR( i, *surface ) && indices.presentFamily == UINT32_MAX )
      {
        indices.presentFamily = i;
      }

      if ( ( props.queueFlags & vk::QueueFlagBits::eCompute ) && indices.computeFamily == UINT32_MAX )
      {
        indices.computeFamily = i;
      }
    }

    if ( indices.computeFamily == UINT32_MAX && indices.graphicsFamily != UINT32_MAX )
    {
      const auto & graphicsProps = queueFamilyProperties[indices.graphicsFamily];
      if ( graphicsProps.queueFlags & vk::QueueFlagBits::eCompute )
      {
        indices.computeFamily = indices.graphicsFamily;
      }
    }

    if ( !indices.isComplete() )
    {
      throw std::runtime_error( "Required queue families not found." );
    }
    isDebug( std::println( "Graphics Queue Family Index: {}", indices.graphicsFamily ); );
    isDebug( std::println( "Present Queue Family Index: {}", indices.presentFamily ); );
    isDebug( std::println( "Compute Queue Family Index: {}", indices.computeFamily ); );
    return indices;
  }

  DeviceBundle createDeviceWithQueues( const vk::raii::PhysicalDevice & physicalDevice, const QueueFamilyIndices & indices )
  {
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };
    if ( indices.computeFamily != UINT32_MAX )
    {
      uniqueQueueFamilies.insert( indices.computeFamily );
    }

    float                                  queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve( uniqueQueueFamilies.size() );
    for ( uint32_t queueFamily : uniqueQueueFamilies )
    {
      queueCreateInfos.push_back( vk::DeviceQueueCreateInfo{}.setQueueFamilyIndex( queueFamily ).setQueueCount( 1 ).setPQueuePriorities( &queuePriority ) );
    }

    vk::DeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.setQueueCreateInfos( queueCreateInfos );
    deviceCreateInfo.setPEnabledExtensionNames( core::deviceExtensions );

    DeviceBundle bundle{};
    bundle.indices       = indices;
    bundle.device        = vk::raii::Device( physicalDevice, deviceCreateInfo );
    bundle.graphicsQueue = vk::raii::Queue( bundle.device, indices.graphicsFamily, 0 );
    bundle.presentQueue  = vk::raii::Queue( bundle.device, indices.presentFamily, 0 );
    if ( indices.computeFamily != UINT32_MAX )
    {
      bundle.computeQueue = vk::raii::Queue( bundle.device, indices.computeFamily, 0 );
    }

    return bundle;
  }

  SwapchainSupportDetails querySwapchainSupport( const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::SurfaceKHR & surface )
  {
    SwapchainSupportDetails details{};
    details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR( *surface );
    details.formats      = physicalDevice.getSurfaceFormatsKHR( *surface );
    details.presentModes = physicalDevice.getSurfacePresentModesKHR( *surface );
    return details;
  }

  vk::SurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<vk::SurfaceFormatKHR> & availableFormats )
  {
    for ( const auto & availableFormat : availableFormats )
    {
      if ( availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear )
      {
        return availableFormat;
      }
    }
    return availableFormats.empty() ? vk::SurfaceFormatKHR{} : availableFormats.front();
  }

  vk::PresentModeKHR chooseSwapPresentMode( const std::vector<vk::PresentModeKHR> & availablePresentModes )
  {
    for ( const auto & mode : availablePresentModes )
    {
      if ( mode == core::preferedPresentationMode )
      {
        return mode;
      }
    }
    return vk::PresentModeKHR::eFifo;
  }

  vk::Extent2D chooseSwapExtent( const vk::SurfaceCapabilitiesKHR & capabilities, const vk::Extent2D & desiredExtent )
  {
    if ( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() )
    {
      return capabilities.currentExtent;
    }
    vk::Extent2D actualExtent = desiredExtent;
    actualExtent.width        = std::max( capabilities.minImageExtent.width, std::min( capabilities.maxImageExtent.width, actualExtent.width ) );
    actualExtent.height       = std::max( capabilities.minImageExtent.height, std::min( capabilities.maxImageExtent.height, actualExtent.height ) );
    return actualExtent;
  }

  SwapchainBundle createSwapchain(
    const vk::raii::PhysicalDevice & physicalDevice,
    const vk::raii::Device &         device,
    const vk::raii::SurfaceKHR &     surface,
    const vk::Extent2D &             desiredExtent,
    const QueueFamilyIndices &       indices )
  {
    SwapchainSupportDetails support = querySwapchainSupport( physicalDevice, surface );

    if ( support.formats.empty() || support.presentModes.empty() )
    {
      throw std::runtime_error( "Swapchain support is insufficient." );
    }

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat( support.formats );
    vk::PresentModeKHR   presentMode   = chooseSwapPresentMode( support.presentModes );
    vk::Extent2D         extent        = chooseSwapExtent( support.capabilities, desiredExtent );

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if ( support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount )
    {
      imageCount = support.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface          = *surface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;

    uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };
    if ( indices.graphicsFamily != indices.presentFamily )
    {
      createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else
    {
      createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform   = support.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;

    SwapchainBundle bundle{};
    bundle.swapchain   = vk::raii::SwapchainKHR( device, createInfo );
    bundle.imageFormat = surfaceFormat.format;
    bundle.extent      = extent;

    bundle.images = bundle.swapchain.getImages();
    bundle.imageViews.reserve( bundle.images.size() );
    for ( const vk::Image & image : bundle.images )
    {
      vk::ImageViewCreateInfo viewInfo{};
      viewInfo.image                           = image;
      viewInfo.viewType                        = vk::ImageViewType::e2D;
      viewInfo.format                          = bundle.imageFormat;
      viewInfo.components.r                    = vk::ComponentSwizzle::eIdentity;
      viewInfo.components.g                    = vk::ComponentSwizzle::eIdentity;
      viewInfo.components.b                    = vk::ComponentSwizzle::eIdentity;
      viewInfo.components.a                    = vk::ComponentSwizzle::eIdentity;
      viewInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
      viewInfo.subresourceRange.baseMipLevel   = 0;
      viewInfo.subresourceRange.levelCount     = 1;
      viewInfo.subresourceRange.baseArrayLayer = 0;
      viewInfo.subresourceRange.layerCount     = 1;
      bundle.imageViews.emplace_back( device, viewInfo );
    }

    return bundle;
  }

}  // namespace core
