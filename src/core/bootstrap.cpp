#include "bootstrap.hpp"

#include <string>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_structs.hpp>

// #include <vulkan/vulkan_core.h>

namespace core
{

  vk::raii::Instance createInstance( vk::raii::Context & context, const std::string & appName, const std::string & engineName )
  {
    vk::ApplicationInfo applicationInfo( appName.c_str(), 1, engineName.c_str(), 1, VK_API_VERSION_1_4 );

    vk::InstanceCreateInfo instanceCreateInfo( {}, &applicationInfo, {}, core::InstanceExtensions );
    return vk::raii::Instance( context, instanceCreateInfo );
  }

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
        isDebug( std::println( "device selected {}", (std::string)device.getProperties().deviceName ); );

        return device;
      }
    }
    isDebug( std::println( "device selected {}", (std::string)devices.front().getProperties().deviceName ); );

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
      isDebug( std::println( "Queue Family Index: {}", i ); );
      isDebug( std::println( "Queue Flags: {}", vk::to_string( props.queueFlags ) ); );

      if ( ( props.queueFlags & vk::QueueFlagBits::eGraphics ) && !indices.graphicsFamily.has_value() )
      {
        indices.graphicsFamily = i;
        if ( !physicalDevice.getSurfaceSupportKHR( i, *surface ) )
        {
          throw std::runtime_error( "Graphics Queue Family does not support surface presentation." );
        }
        if ( !( props.queueFlags & vk::QueueFlagBits::eCompute ) )
        {
          throw std::runtime_error( "Graphics Queue Family does not support compute." );
        }
      }
      if ( ( props.queueFlags & vk::QueueFlagBits::eCompute ) )
      {
        if ( indices.graphicsFamily.has_value() && i != indices.graphicsFamily.value() )
        {
          indices.computeFamily = i;
        }
      }
    }

    if ( !indices.isComplete() )
    {
      throw std::runtime_error( "Required queue families not found." );
    }
    isDebug( std::println( "Graphics Queue Family Index: {}", indices.graphicsFamily.value() ); );
    isDebug( std::println( "Compute Queue Family Index: {}", indices.computeFamily.value() ); );
    return indices;
  }

  DeviceBundle createDeviceWithQueues( const vk::raii::PhysicalDevice & physicalDevice, const QueueFamilyIndices & indices )
  {
    // --- Queues setup ---
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.computeFamily.value() };

    float                                  queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve( uniqueQueueFamilies.size() );
    for ( uint32_t queueFamily : uniqueQueueFamilies )
    {
      queueCreateInfos.push_back( vk::DeviceQueueCreateInfo{}.setQueueFamilyIndex( queueFamily ).setQueueCount( 1 ).setPQueuePriorities( &queuePriority ) );
    }

    // --- Query supported features ---
    vk::PhysicalDeviceVulkan14Features supported14{};
    vk::PhysicalDeviceVulkan13Features supported13{};
    vk::PhysicalDeviceVulkan12Features supported12{};
    vk::PhysicalDeviceVulkan11Features supported11{};
    vk::PhysicalDeviceFeatures2        supported2{};

    supported2.pNext  = &supported11;
    supported11.pNext = &supported12;
    supported12.pNext = &supported13;
    supported13.pNext = &supported14;

    vkGetPhysicalDeviceFeatures2( static_cast<VkPhysicalDevice>( *physicalDevice ), reinterpret_cast<VkPhysicalDeviceFeatures2 *>( &supported2 ) );

    // --- Enable selectively ---
    vk::PhysicalDeviceVulkan14Features vulkan14Features{};
    vulkan14Features.pushDescriptor = supported14.pushDescriptor;
    vulkan14Features.maintenance5   = supported14.maintenance5;
    vulkan14Features.maintenance6   = supported14.maintenance6;
    vulkan14Features.smoothLines    = supported14.smoothLines;

    vk::PhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.dynamicRendering = supported13.dynamicRendering;
    vulkan13Features.synchronization2 = supported13.synchronization2;
    vulkan13Features.maintenance4     = supported13.maintenance4;
    vulkan14Features.pNext            = &vulkan13Features;

    vk::PhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.descriptorIndexing                       = supported12.descriptorIndexing;
    vulkan12Features.runtimeDescriptorArray                   = supported12.runtimeDescriptorArray;
    vulkan12Features.descriptorBindingPartiallyBound          = supported12.descriptorBindingPartiallyBound;
    vulkan12Features.descriptorBindingVariableDescriptorCount = supported12.descriptorBindingVariableDescriptorCount;
    vulkan12Features.bufferDeviceAddress                      = supported12.bufferDeviceAddress;
    vulkan12Features.timelineSemaphore                        = supported12.timelineSemaphore;
    vulkan13Features.pNext                                    = &vulkan12Features;

    vk::PhysicalDeviceVulkan11Features vulkan11Features{};
    vulkan11Features.shaderDrawParameters = supported11.shaderDrawParameters;
    vulkan12Features.pNext                = &vulkan11Features;

    // --- Core features ---
    vk::PhysicalDeviceFeatures2 deviceFeatures2{};

    vk::PhysicalDeviceFeatures features{};
    features.samplerAnisotropy = supported2.features.samplerAnisotropy;
    features.fillModeNonSolid  = supported2.features.fillModeNonSolid;
    features.wideLines         = supported2.features.wideLines;

    deviceFeatures2.features = features;
    vulkan11Features.pNext   = &deviceFeatures2;

    // --- Extensions ---
    std::vector<const char *> finalExtensions = core::deviceExtensions;
    auto                      avail           = physicalDevice.enumerateDeviceExtensionProperties();
    // Declare at function scope (before the extension loop)
    vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageableFeatures{};
    vk::PhysicalDeviceShaderObjectFeaturesEXT              shaderObject{};
    bool                                                   enablePageable     = false;
    bool                                                   enableShaderObject = false;

    // In your loop:
    for ( auto const & ep : avail )
    {
      std::string extName( ep.extensionName );
      if ( extName == VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME )
      {
        finalExtensions.push_back( VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME );
        finalExtensions.push_back( VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME );
        enablePageable = true;
      }
      if ( extName == VK_EXT_SHADER_OBJECT_EXTENSION_NAME )
      {
        finalExtensions.push_back( VK_EXT_SHADER_OBJECT_EXTENSION_NAME );
        enableShaderObject = true;
      }
    }
    // After the loop, build the pNext chain properly:
    void * chainTail = deviceFeatures2.pNext;

    if ( enableShaderObject )
    {
      shaderObject.shaderObject = VK_TRUE;
      shaderObject.pNext        = chainTail;
      chainTail                 = &shaderObject;
    }

    if ( enablePageable )
    {
      pageableFeatures.pageableDeviceLocalMemory = VK_TRUE;
      pageableFeatures.pNext                     = chainTail;
      chainTail                                  = &pageableFeatures;
    }

    // Update the head of the chain
    deviceFeatures2.pNext = chainTail;
    // --- Device create info ---
    vk::DeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.setQueueCreateInfos( queueCreateInfos );
    deviceCreateInfo.setPEnabledExtensionNames( finalExtensions );
    deviceCreateInfo.pNext = &vulkan14Features;

    // --- Create device + queues ---
    DeviceBundle bundle{};
    bundle.indices       = indices;
    bundle.device        = vk::raii::Device( physicalDevice, deviceCreateInfo );
    bundle.graphicsQueue = vk::raii::Queue( bundle.device, indices.graphicsFamily.value(), 0 );

    if ( indices.computeFamily.has_value() )
    {
      bundle.computeQueue = vk::raii::Queue( bundle.device, indices.computeFamily.value(), 0 );
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
    const QueueFamilyIndices &       indices,
    const vk::raii::SwapchainKHR *   oldSwapchain )
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
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    createInfo.preTransform     = support.capabilities.currentTransform;
    createInfo.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode      = presentMode;
    createInfo.clipped          = VK_TRUE;
    if ( oldSwapchain && static_cast<VkSwapchainKHR>( **oldSwapchain ) != VK_NULL_HANDLE )
    {
      createInfo.oldSwapchain = **oldSwapchain;
    }

    SwapchainBundle bundle{};
    bundle.swapchain   = vk::raii::SwapchainKHR( device, createInfo );
    bundle.imageFormat = surfaceFormat.format;
    bundle.extent      = extent;

    bundle.images = bundle.swapchain.getImages();
    bundle.imageViews.reserve( bundle.images.size() );
    for ( const vk::Image & image : bundle.images )
    {
      vk::ImageViewCreateInfo viewInfo{};                                          // Create an image view create info struct
      viewInfo.image                           = image;                            // The image to create a view for
      viewInfo.viewType                        = vk::ImageViewType::e2D;           // 2D texture view
      viewInfo.format                          = bundle.imageFormat;               // Format matches swapchain image format
      viewInfo.components.r                    = vk::ComponentSwizzle::eIdentity;  // No swizzle for R
      viewInfo.components.g                    = vk::ComponentSwizzle::eIdentity;  // No swizzle for G
      viewInfo.components.b                    = vk::ComponentSwizzle::eIdentity;  // No swizzle for B
      viewInfo.components.a                    = vk::ComponentSwizzle::eIdentity;  // No swizzle for A
      viewInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;  // View color aspect
      viewInfo.subresourceRange.baseMipLevel   = 0;                                // Start at first mip level
      viewInfo.subresourceRange.levelCount     = 1;                                // Only one mip level
      viewInfo.subresourceRange.baseArrayLayer = 0;                                // Start at first array layer
      viewInfo.subresourceRange.layerCount     = 1;                                // Only one array layer
      bundle.imageViews.emplace_back( device, viewInfo );                          // Create and store the image view
    }

    return bundle;
  }

  std::vector<uint32_t> readSpirvFile( const std::string & filePath )
  {
    std::ifstream file( filePath, std::ios::ate | std::ios::binary );
    if ( !file.is_open() )
    {
      throw std::runtime_error( std::string( "failed to open SPIR-V file: " ) + filePath );
    }

    size_t fileSize = static_cast<size_t>( file.tellg() );
    if ( fileSize % 4 != 0 )
    {
      throw std::runtime_error( std::string( "SPIR-V file size is not a multiple of 4: " ) + filePath );
    }
    std::vector<uint32_t> buffer( fileSize / 4 );
    file.seekg( 0 );
    file.read( reinterpret_cast<char *>( buffer.data() ), fileSize );
    return buffer;
  }

  vk::raii::ShaderModule createShaderModule( const vk::raii::Device & device, const std::vector<uint32_t> & spirv )
  {
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = spirv.size() * sizeof( uint32_t );
    createInfo.pCode    = spirv.data();
    return vk::raii::ShaderModule( device, createInfo );
  }
}  // namespace core
