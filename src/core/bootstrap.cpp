#include "bootstrap.hpp"

#include <algorithm>
#include <limits>
#include <set>
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
        if ( !( props.queueFlags & vk::QueueFlagBits::eCompute ) )
        {
          throw std::runtime_error( "Graphics Queue Family does not support compute." );
        }
      }
      if ( physicalDevice.getSurfaceSupportKHR( i, *surface ) && !indices.presentFamily.has_value() )
      {
        indices.presentFamily = i;
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
    isDebug( std::println( "Present Queue Family Index: {}", indices.presentFamily.value() ); );
    isDebug( std::println( "Compute Queue Family Index: {}", indices.computeFamily.value() ); );
    return indices;
  }

  DeviceBundle createDeviceWithQueues( const vk::raii::PhysicalDevice & physicalDevice, const QueueFamilyIndices & indices )
  {
    // --- Queues setup ---
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.computeFamily.value() };

    float                                  queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve( uniqueQueueFamilies.size() );
    for ( uint32_t queueFamily : uniqueQueueFamilies )
    {
      queueCreateInfos.push_back( vk::DeviceQueueCreateInfo{}.setQueueFamilyIndex( queueFamily ).setQueueCount( 1 ).setPQueuePriorities( &queuePriority ) );
    }

    // --- Query supported features ---
    vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT supportedExtendedDynamicState3{};
    vk::PhysicalDeviceVulkan14Features                 supported14{};
    vk::PhysicalDeviceVulkan13Features                 supported13{};
    vk::PhysicalDeviceVulkan12Features                 supported12{};
    vk::PhysicalDeviceVulkan11Features                 supported11{};
    vk::PhysicalDeviceFeatures2                        supported2{};

    supported2.setPNext( &supported11 );
    supported11.setPNext( &supported12 );
    supported12.setPNext( &supported13 );
    supported13.setPNext( &supported14 );
    supported14.setPNext( &supportedExtendedDynamicState3 );

    vkGetPhysicalDeviceFeatures2( static_cast<VkPhysicalDevice>( *physicalDevice ), reinterpret_cast<VkPhysicalDeviceFeatures2 *>( &supported2 ) );

    // --- Enable selectively ---
    vk::PhysicalDeviceVulkan14Features vulkan14Features{};
    vulkan14Features.setPushDescriptor( supported14.pushDescriptor );
    vulkan14Features.setMaintenance5( supported14.maintenance5 );
    vulkan14Features.setMaintenance6( supported14.maintenance6 );
    vulkan14Features.setSmoothLines( supported14.smoothLines );

    vk::PhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.setDynamicRendering( supported13.dynamicRendering );
    vulkan13Features.setSynchronization2( supported13.synchronization2 );
    vulkan13Features.setMaintenance4( supported13.maintenance4 );
    vulkan14Features.setPNext( &vulkan13Features );

    vk::PhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.setDescriptorIndexing( supported12.descriptorIndexing );
    vulkan12Features.setRuntimeDescriptorArray( supported12.runtimeDescriptorArray );
    vulkan12Features.setDescriptorBindingPartiallyBound( supported12.descriptorBindingPartiallyBound );
    vulkan12Features.setDescriptorBindingVariableDescriptorCount( supported12.descriptorBindingVariableDescriptorCount );
    vulkan12Features.setBufferDeviceAddress( supported12.bufferDeviceAddress );
    vulkan12Features.setTimelineSemaphore( supported12.timelineSemaphore );
    vulkan13Features.setPNext( &vulkan12Features );

    vk::PhysicalDeviceVulkan11Features vulkan11Features{};
    vulkan11Features.setShaderDrawParameters( supported11.shaderDrawParameters );
    vulkan12Features.setPNext( &vulkan11Features );

    // --- Core features ---
    vk::PhysicalDeviceFeatures2 deviceFeatures2{};

    vk::PhysicalDeviceFeatures features{};
    features.setSamplerAnisotropy( supported2.features.samplerAnisotropy );
    features.setFillModeNonSolid( supported2.features.fillModeNonSolid );
    features.setWideLines( supported2.features.wideLines );

    deviceFeatures2.setFeatures( features );
    vulkan11Features.setPNext( &deviceFeatures2 );

    // --- Extensions ---
    std::vector<const char *> finalExtensions = core::deviceExtensions;
    auto                      avail           = physicalDevice.enumerateDeviceExtensionProperties();
    // Declare at function scope (before the extension loop)
    vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageableFeatures{};
    vk::PhysicalDeviceShaderObjectFeaturesEXT              shaderObject{};
    vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT     extendedDynamicState3{};
    vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT     swapchainMaintenance1{};

    bool                                                   enablePageable          = false;
    bool                                                   enableShaderObject      = false;
    bool                                                   enableExtendedDynState3 = false;
    bool                                                   enableSwapchainMaintenance1 = false;


    // In your loop:
    for ( auto const & ep : avail )
    {
      // std::println( "Extension: {}", (std::string)ep.extensionName );
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
      if ( extName == VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME )
      {
        finalExtensions.push_back( VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME );
        enableExtendedDynState3 = true;
      }
      if ( extName == VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME )
      {
        finalExtensions.push_back( VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME );
        enableSwapchainMaintenance1 = true;
      }
    }
    // finalExtensions.push_back( VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME );
    // enableSwapchainMaintenance1 = true;
    // After the loop, build the pNext chain properly:
    void * chainTail = deviceFeatures2.pNext;

    if ( enableShaderObject )
    {
      shaderObject.setShaderObject( VK_TRUE );
      shaderObject.setPNext( chainTail );
      chainTail = &shaderObject;
    }

    if ( enablePageable )
    {
      pageableFeatures.setPageableDeviceLocalMemory( VK_TRUE );
      pageableFeatures.setPNext( chainTail );
      chainTail = &pageableFeatures;
    }

    if ( enableExtendedDynState3 )
    {
      // Enable all Extended Dynamic State 3 features
      extendedDynamicState3.setExtendedDynamicState3TessellationDomainOrigin( supportedExtendedDynamicState3.extendedDynamicState3TessellationDomainOrigin );
      extendedDynamicState3.setExtendedDynamicState3DepthClampEnable( supportedExtendedDynamicState3.extendedDynamicState3DepthClampEnable );
      extendedDynamicState3.setExtendedDynamicState3PolygonMode( supportedExtendedDynamicState3.extendedDynamicState3PolygonMode );
      extendedDynamicState3.setExtendedDynamicState3RasterizationSamples( supportedExtendedDynamicState3.extendedDynamicState3RasterizationSamples );
      extendedDynamicState3.setExtendedDynamicState3SampleMask( supportedExtendedDynamicState3.extendedDynamicState3SampleMask );
      extendedDynamicState3.setExtendedDynamicState3AlphaToCoverageEnable( supportedExtendedDynamicState3.extendedDynamicState3AlphaToCoverageEnable );
      extendedDynamicState3.setExtendedDynamicState3AlphaToOneEnable( supportedExtendedDynamicState3.extendedDynamicState3AlphaToOneEnable );
      extendedDynamicState3.setExtendedDynamicState3LogicOpEnable( supportedExtendedDynamicState3.extendedDynamicState3LogicOpEnable );
      extendedDynamicState3.setExtendedDynamicState3ColorBlendEnable( supportedExtendedDynamicState3.extendedDynamicState3ColorBlendEnable );
      extendedDynamicState3.setExtendedDynamicState3ColorBlendEquation( supportedExtendedDynamicState3.extendedDynamicState3ColorBlendEquation );
      extendedDynamicState3.setExtendedDynamicState3ColorWriteMask( supportedExtendedDynamicState3.extendedDynamicState3ColorWriteMask );
      extendedDynamicState3.setExtendedDynamicState3RasterizationStream( supportedExtendedDynamicState3.extendedDynamicState3RasterizationStream );
      extendedDynamicState3.setExtendedDynamicState3ConservativeRasterizationMode( supportedExtendedDynamicState3.extendedDynamicState3ConservativeRasterizationMode );
      extendedDynamicState3.setExtendedDynamicState3ExtraPrimitiveOverestimationSize( supportedExtendedDynamicState3.extendedDynamicState3ExtraPrimitiveOverestimationSize );
      extendedDynamicState3.setExtendedDynamicState3DepthClipEnable( supportedExtendedDynamicState3.extendedDynamicState3DepthClipEnable );
      extendedDynamicState3.setExtendedDynamicState3SampleLocationsEnable( supportedExtendedDynamicState3.extendedDynamicState3SampleLocationsEnable );
      extendedDynamicState3.setExtendedDynamicState3ColorBlendAdvanced( supportedExtendedDynamicState3.extendedDynamicState3ColorBlendAdvanced );
      extendedDynamicState3.setExtendedDynamicState3ProvokingVertexMode( supportedExtendedDynamicState3.extendedDynamicState3ProvokingVertexMode );
      extendedDynamicState3.setExtendedDynamicState3LineRasterizationMode( supportedExtendedDynamicState3.extendedDynamicState3LineRasterizationMode );
      extendedDynamicState3.setExtendedDynamicState3LineStippleEnable( supportedExtendedDynamicState3.extendedDynamicState3LineStippleEnable );
      extendedDynamicState3.setExtendedDynamicState3DepthClipNegativeOneToOne( supportedExtendedDynamicState3.extendedDynamicState3DepthClipNegativeOneToOne );
      extendedDynamicState3.setExtendedDynamicState3ViewportWScalingEnable( supportedExtendedDynamicState3.extendedDynamicState3ViewportWScalingEnable );
      extendedDynamicState3.setExtendedDynamicState3ViewportSwizzle( supportedExtendedDynamicState3.extendedDynamicState3ViewportSwizzle );
      extendedDynamicState3.setExtendedDynamicState3CoverageToColorEnable( supportedExtendedDynamicState3.extendedDynamicState3CoverageToColorEnable );
      extendedDynamicState3.setExtendedDynamicState3CoverageToColorLocation( supportedExtendedDynamicState3.extendedDynamicState3CoverageToColorLocation );
      extendedDynamicState3.setExtendedDynamicState3CoverageModulationMode( supportedExtendedDynamicState3.extendedDynamicState3CoverageModulationMode );
      extendedDynamicState3.setExtendedDynamicState3CoverageModulationTableEnable( supportedExtendedDynamicState3.extendedDynamicState3CoverageModulationTableEnable );
      extendedDynamicState3.setExtendedDynamicState3CoverageModulationTable( supportedExtendedDynamicState3.extendedDynamicState3CoverageModulationTable );
      extendedDynamicState3.setExtendedDynamicState3CoverageReductionMode( supportedExtendedDynamicState3.extendedDynamicState3CoverageReductionMode );
      extendedDynamicState3.setExtendedDynamicState3RepresentativeFragmentTestEnable( supportedExtendedDynamicState3.extendedDynamicState3RepresentativeFragmentTestEnable );
      extendedDynamicState3.setExtendedDynamicState3ShadingRateImageEnable( supportedExtendedDynamicState3.extendedDynamicState3ShadingRateImageEnable );
      extendedDynamicState3.setPNext( chainTail );
      chainTail = &extendedDynamicState3;
    }
    if ( enableSwapchainMaintenance1 )
    {
      swapchainMaintenance1.setSwapchainMaintenance1( VK_TRUE );
      swapchainMaintenance1.setPNext( chainTail );
      chainTail = &swapchainMaintenance1;
    }

    // Update the head of the chain
    deviceFeatures2.setPNext( chainTail );
    // --- Device create info ---
    vk::DeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.setQueueCreateInfos( queueCreateInfos );
    deviceCreateInfo.setPEnabledExtensionNames( finalExtensions );
    deviceCreateInfo.setPNext( &vulkan14Features );

    // --- Create device + queues ---
    DeviceBundle bundle{};
    bundle.indices       = indices;
    bundle.device        = vk::raii::Device( physicalDevice, deviceCreateInfo );
    bundle.graphicsQueue = vk::raii::Queue( bundle.device, indices.graphicsFamily.value(), 0 );
    bundle.presentQueue  = vk::raii::Queue( bundle.device, indices.presentFamily.value(), 0 );

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
    createInfo.setSurface( *surface );
    createInfo.setMinImageCount( imageCount );
    createInfo.setImageFormat( surfaceFormat.format );
    createInfo.setImageColorSpace( surfaceFormat.colorSpace );
    createInfo.setImageExtent( extent );
    createInfo.setImageArrayLayers( 1 );
    createInfo.setImageUsage( vk::ImageUsageFlagBits::eColorAttachment );
    
    // Configure sharing mode based on queue family indices
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    if ( indices.graphicsFamily.value() != indices.presentFamily.value() )
    {
      createInfo.setImageSharingMode( vk::SharingMode::eConcurrent );
      createInfo.setQueueFamilyIndexCount( 2 );
      createInfo.setPQueueFamilyIndices( queueFamilyIndices );
    }
    else
    {
      createInfo.setImageSharingMode( vk::SharingMode::eExclusive );
    }
    
    createInfo.setPreTransform( support.capabilities.currentTransform );
    createInfo.setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque );
    createInfo.setPresentMode( presentMode );
    createInfo.setClipped( VK_TRUE );
    if ( oldSwapchain && static_cast<VkSwapchainKHR>( **oldSwapchain ) != VK_NULL_HANDLE )
    {
      createInfo.setOldSwapchain( **oldSwapchain );
    }

    // Add VkSwapchainPresentModesCreateInfoKHR to inform implementation about swapchain maintenance awareness
    vk::SwapchainPresentModesCreateInfoEXT presentModesInfo{};
    presentModesInfo.setPresentModeCount( 1 );
    presentModesInfo.setPPresentModes( &presentMode );
    createInfo.setPNext( &presentModesInfo );

    SwapchainBundle bundle{};
    bundle.swapchain   = vk::raii::SwapchainKHR( device, createInfo );
    bundle.imageFormat = surfaceFormat.format;
    bundle.extent      = extent;

    bundle.images = bundle.swapchain.getImages();
    bundle.imageViews.reserve( bundle.images.size() );
    for ( const vk::Image & image : bundle.images )
    {
      vk::ImageViewCreateInfo viewInfo{};  // Create an image view create info struct
      vk::ComponentMapping    components{};
      components.setR( vk::ComponentSwizzle::eIdentity );  // No swizzle for R
      components.setG( vk::ComponentSwizzle::eIdentity );  // No swizzle for G
      components.setB( vk::ComponentSwizzle::eIdentity );  // No swizzle for B
      components.setA( vk::ComponentSwizzle::eIdentity );  // No swizzle for A
      vk::ImageSubresourceRange subresourceRange{};
      subresourceRange.setAspectMask( vk::ImageAspectFlagBits::eColor );  // View color aspect
      subresourceRange.setBaseMipLevel( 0 );                              // Start at first mip level
      subresourceRange.setLevelCount( 1 );                                // Only one mip level
      subresourceRange.setBaseArrayLayer( 0 );                            // Start at first array layer
      subresourceRange.setLayerCount( 1 );                                // Only one array layer
      viewInfo.setImage( image );                                         // The image to create a view for
      viewInfo.setViewType( vk::ImageViewType::e2D );                     // 2D texture view
      viewInfo.setFormat( bundle.imageFormat );                           // Format matches swapchain image format
      viewInfo.setComponents( components );
      viewInfo.setSubresourceRange( subresourceRange );
      bundle.imageViews.emplace_back( device, viewInfo );  // Create and store the image view
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
    createInfo.setCodeSize( spirv.size() * sizeof( uint32_t ) );
    createInfo.setPCode( spirv.data() );
    return vk::raii::ShaderModule( device, createInfo );
  }
}  // namespace core
