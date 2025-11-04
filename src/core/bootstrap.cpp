#include "bootstrap.hpp"

#include "settings.hpp"
#include "vulkan/vulkan.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <print>
#include <set>
#include <string>

namespace core
{

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
    // Set up error callback before initialization to capture any errors
    static std::string lastGlfwError;
    glfwSetErrorCallback( []( int error, const char * msg ) {
      lastGlfwError = std::string( "GLFW Error (" ) + std::to_string( error ) + "): " + ( msg ? msg : "Unknown error" );
      std::cerr << "[GLFW] " << lastGlfwError << std::endl;
    } );

    isDebug( std::println( "[DisplayBundle] Initializing GLFW for window: '{}' ({}x{})", windowName, extent_.width, extent_.height ) );

    if ( !glfwInit() )
    {
      std::string errorMsg = "Failed to initialize GLFW";
      if ( !lastGlfwError.empty() )
      {
        errorMsg += ": " + lastGlfwError;
      }
      else
      {
        errorMsg += " (no error details available)";
      }
      std::println( "[DisplayBundle] ERROR: {}", errorMsg );
      throw std::runtime_error( errorMsg );
    }

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

  DeviceBundle createDeviceWithQueues(
    const vk::raii::PhysicalDevice & physicalDevice, const QueueFamilyIndices & indices, const void * pNextFeatureChain, const std::vector<const char *> & finalExtensions )
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

    // --- Device create info ---
    vk::DeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.setQueueCreateInfos( queueCreateInfos );
    deviceCreateInfo.setPEnabledExtensionNames( finalExtensions );
    deviceCreateInfo.setPNext( pNextFeatureChain );

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
    createInfo.setImageUsage( vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst );

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

    // lol this not supported
    // vk::SwapchainPresentScalingCreateInfoEXT presentScalingInfo{};
    // presentScalingInfo.setPresentGravityX(vk::PresentGravityFlagBitsKHR::eCentered);
    // presentScalingInfo.setPresentGravityY(vk::PresentGravityFlagBitsKHR::eCentered);
    // presentScalingInfo.setScalingBehavior(vk::PresentScalingFlagBitsKHR::eAspectRatioStretch);

    // Add VkSwapchainPresentModesCreateInfoKHR to inform implementation about swapchain maintenance awareness
    vk::SwapchainPresentModesCreateInfoEXT presentModesInfo{};
    presentModesInfo.setPresentModeCount( static_cast<uint32_t>( support.presentModes.size() ) );
    presentModesInfo.setPPresentModes( support.presentModes.data() );
    // presentModesInfo.setPNext( &presentScalingInfo );
    createInfo.setPNext( &presentModesInfo );

    SwapchainBundle bundle{};
    bundle.swapchain   = vk::raii::SwapchainKHR( device, createInfo );
    bundle.imageFormat = surfaceFormat.format;
    bundle.extent      = extent;

    bundle.images = bundle.swapchain.getImages();
    bundle.imageViews.reserve( bundle.images.size() );
    for ( const vk::Image & image : bundle.images )
    {
      vk::ComponentMapping components{};
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

      vk::ImageViewCreateInfo viewInfo{};              // Create an image view create info struct
      viewInfo.setImage( image );                      // The image to create a view for
      viewInfo.setViewType( vk::ImageViewType::e2D );  // 2D texture view
      viewInfo.setFormat( bundle.imageFormat );        // Format matches swapchain image format
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
