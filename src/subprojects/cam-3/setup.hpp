#pragma once

#include "state.hpp"

#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <print>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_NONE
#include "features.hpp"
#include "helper.hpp"

#include <GLFW/glfw3.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vk_mem_alloc.h>

// #include <vulkan/vulkan_core.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#if defined( DEBUG ) || !defined( NDEBUG )
#  define isDebug( code ) code
#else
#  define isDebug( code )
#endif

namespace core
{

  // ---------------------------------------------------------------------------
  // Physical device selection
  // ---------------------------------------------------------------------------

  [[nodiscard]] inline vk::raii::PhysicalDevice selectPhysicalDevice( const vk::raii::PhysicalDevices & devices )
  {
    if ( devices.empty() )
      throw std::runtime_error( "No Vulkan physical devices found." );

    // Prefer discrete GPU
    for ( const auto & device : devices )
    {
      vk::PhysicalDeviceProperties props = device.getProperties();
      if ( props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
      {
        isDebug( std::println( "device selected {}", (std::string)props.deviceName ); );
        return device;
      }
    }

    isDebug( std::println( "device selected {}", (std::string)devices.front().getProperties().deviceName ); );
    return devices.front();
  }

  // ---------------------------------------------------------------------------
  // DisplayBundle (window + surface)
  // ---------------------------------------------------------------------------
  [[nodiscard]] std::unique_ptr<GLFWwindow, void ( * )( GLFWwindow * )> createWindow( vk::raii::Instance const & instance );

  [[nodiscard]] vk::raii::SurfaceKHR createWindowSurface( const vk::raii::Instance & instance, GLFWwindow * window );

  inline void glfwDestructor( GLFWwindow * w )
  {
    if ( w )
    {
      glfwDestroyWindow( w );
      glfwTerminate();
    }
  }

  // ---------------------------------------------------------------------------
  // Queue families
  // ---------------------------------------------------------------------------

  struct QueueFamilyIndices
  {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;

    [[nodiscard]] inline bool isComplete() const
    {
      return graphicsFamily && presentFamily && computeFamily;
    }
  };

  [[nodiscard]] inline QueueFamilyIndices findQueueFamilies( const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::SurfaceKHR & surface )
  {
    QueueFamilyIndices indices;
    auto               queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    for ( uint32_t i = 0; i < queueFamilyProperties.size(); ++i )
    {
      const auto & props = queueFamilyProperties[i];
      isDebug( std::println( "Queue Family Index: {}", i ); );
      isDebug( std::println( "Queue Flags: {}", vk::to_string( props.queueFlags ) ); );

      if ( ( props.queueFlags & vk::QueueFlagBits::eGraphics ) && !indices.graphicsFamily )
      {
        indices.graphicsFamily = i;
        if ( !( props.queueFlags & vk::QueueFlagBits::eCompute ) )
          throw std::runtime_error( "Graphics Queue Family does not support compute." );
      }

      if ( physicalDevice.getSurfaceSupportKHR( i, *surface ) && !indices.presentFamily )
        indices.presentFamily = i;

      if ( ( props.queueFlags & vk::QueueFlagBits::eCompute ) && indices.graphicsFamily && i != indices.graphicsFamily.value() )
        indices.computeFamily = i;
    }

    if ( !indices.isComplete() )
      throw std::runtime_error( "Required queue families not found." );

    isDebug( std::println( "Graphics Queue Family Index: {}", indices.graphicsFamily.value() ); );
    isDebug( std::println( "Present Queue Family Index: {}", indices.presentFamily.value() ); );
    isDebug( std::println( "Compute Queue Family Index: {}", indices.computeFamily.value() ); );

    return indices;
  }

  // ---------------------------------------------------------------------------
  // Device creation
  // ---------------------------------------------------------------------------

  [[nodiscard]] inline vk::raii::Device createDevice(
    const vk::raii::PhysicalDevice &  physicalDevice,
    const QueueFamilyIndices &        indices,
    const void *                      pNextFeatureChain,
    const std::vector<const char *> & finalExtensions )
  {
    std::set<uint32_t> uniqueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.computeFamily.value() };
    float              queuePriority  = 1.0f;

    std::vector<vk::DeviceQueueCreateInfo> queueInfos;
    for ( uint32_t q : uniqueFamilies )
      queueInfos.push_back( vk::DeviceQueueCreateInfo{}.setQueueFamilyIndex( q ).setQueueCount( 1 ).setPQueuePriorities( &queuePriority ) );

    vk::DeviceCreateInfo info{};
    info.setQueueCreateInfos( queueInfos );
    info.setPEnabledExtensionNames( finalExtensions );
    info.setPNext( pNextFeatureChain );

    return vk::raii::Device( physicalDevice, info );
  }

  // ---------------------------------------------------------------------------
  // Swapchain support and creation
  // ---------------------------------------------------------------------------

  struct SwapchainSupportDetails
  {
    vk::SurfaceCapabilitiesKHR        capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR>   presentModes;
  };

  [[nodiscard]] inline SwapchainSupportDetails querySwapchainSupport( const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::SurfaceKHR & surface )
  {
    return { physicalDevice.getSurfaceCapabilitiesKHR( *surface ),
             physicalDevice.getSurfaceFormatsKHR( *surface ),
             physicalDevice.getSurfacePresentModesKHR( *surface ) };
  }

  [[nodiscard]] inline vk::SurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<vk::SurfaceFormatKHR> & available )
  {
    for ( const auto & f : available )
      if ( f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear )
        return f;
    return available.empty() ? vk::SurfaceFormatKHR{} : available.front();
  }

  [[nodiscard]] inline vk::PresentModeKHR chooseSwapPresentMode( const std::vector<vk::PresentModeKHR> & available )
  {
    for ( auto & m : available )
      if ( m == global::state::presentMode )
        return m;
    return vk::PresentModeKHR::eFifo;
  }

  [[nodiscard]] inline vk::Extent2D chooseSwapExtent( const vk::SurfaceCapabilitiesKHR & caps, const vk::Extent2D & desired )
  {
    if ( caps.currentExtent.width != std::numeric_limits<uint32_t>::max() )
      return caps.currentExtent;

    vk::Extent2D actual = desired;
    actual.width        = std::clamp( actual.width, caps.minImageExtent.width, caps.maxImageExtent.width );
    actual.height       = std::clamp( actual.height, caps.minImageExtent.height, caps.maxImageExtent.height );
    return actual;
  }

  struct SwapchainBundle
  {
    vk::raii::SwapchainKHR           swapchain{ nullptr };
    vk::Format                       imageFormat{ vk::Format::eUndefined };
    vk::Extent2D                     extent{};
    std::vector<vk::Image>           images;
    std::vector<vk::raii::ImageView> imageViews;
  };

  [[nodiscard]] inline SwapchainBundle createSwapchain(
    const vk::raii::PhysicalDevice & physicalDevice,
    const vk::raii::Device &         device,
    const vk::raii::SurfaceKHR &     surface,
    const vk::Extent2D &             desiredExtent,
    const QueueFamilyIndices &       indices,
    const vk::raii::SwapchainKHR *   oldSwapchain = nullptr )
  {
    SwapchainSupportDetails support = querySwapchainSupport( physicalDevice, surface );
    if ( support.formats.empty() || support.presentModes.empty() )
      throw std::runtime_error( "Swapchain support is insufficient." );

    vk::SurfaceFormatKHR format = chooseSwapSurfaceFormat( support.formats );
    vk::PresentModeKHR   mode   = chooseSwapPresentMode( support.presentModes );
    vk::Extent2D         extent = chooseSwapExtent( support.capabilities, desiredExtent );

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if ( support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount )
      imageCount = support.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR info{};
    info.setSurface( *surface )
      .setMinImageCount( imageCount )
      .setImageFormat( format.format )
      .setImageColorSpace( format.colorSpace )
      .setImageExtent( extent )
      .setImageArrayLayers( 1 )
      .setImageUsage( vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst );

    uint32_t qIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    if ( indices.graphicsFamily.value() != indices.presentFamily.value() )
      info.setImageSharingMode( vk::SharingMode::eConcurrent ).setQueueFamilyIndices( qIndices );
    else
      info.setImageSharingMode( vk::SharingMode::eExclusive );

    info.setPreTransform( support.capabilities.currentTransform )
      .setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque )
      .setPresentMode( mode )
      .setClipped( VK_TRUE );
    if ( oldSwapchain && static_cast<VkSwapchainKHR>( **oldSwapchain ) != VK_NULL_HANDLE )
      info.setOldSwapchain( **oldSwapchain );

    vk::SwapchainPresentModesCreateInfoEXT presentModesInfo{};
    presentModesInfo.setPresentModes( support.presentModes );
    info.setPNext( &presentModesInfo );

    SwapchainBundle bundle{};
    bundle.swapchain   = vk::raii::SwapchainKHR( device, info );
    bundle.imageFormat = format.format;
    bundle.extent      = extent;
    bundle.images      = bundle.swapchain.getImages();

    for ( auto & image : bundle.images )
    {
      vk::ImageViewCreateInfo viewInfo{};
      viewInfo.setImage( image )
        .setViewType( vk::ImageViewType::e2D )
        .setFormat( bundle.imageFormat )
        .setComponents( {} )
        .setSubresourceRange( { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } );
      bundle.imageViews.emplace_back( device, viewInfo );
    }

    return bundle;
  }

  // ---------------------------------------------------------------------------
  // SPIR-V loading and shader creation
  // ---------------------------------------------------------------------------

  [[nodiscard]] inline std::vector<uint32_t> readSpirvFile( const std::string & path )
  {
    std::ifstream file( path, std::ios::ate | std::ios::binary );
    if ( !file.is_open() )
      throw std::runtime_error( "failed to open SPIR-V file: " + path );

    size_t size = (size_t)file.tellg();
    if ( size % 4 != 0 )
      throw std::runtime_error( "SPIR-V file size not multiple of 4: " + path );

    std::vector<uint32_t> buffer( size / 4 );
    file.seekg( 0 );
    file.read( reinterpret_cast<char *>( buffer.data() ), size );
    return buffer;
  }

  [[nodiscard]] inline vk::raii::ShaderModule createShaderModule( const vk::raii::Device & device, const std::vector<uint32_t> & spirv )
  {
    vk::ShaderModuleCreateInfo info{};
    info.setCodeSize( spirv.size() * sizeof( uint32_t ) );
    info.setPCode( spirv.data() );
    return vk::raii::ShaderModule( device, info );
  }

  inline vk::ApplicationInfo applicationInfo = vk::ApplicationInfo()
                                                 .setPApplicationName( global::state::AppName.data() )
                                                 .setApplicationVersion( VK_MAKE_VERSION( 1, 0, 0 ) )
                                                 .setPEngineName( global::state::EngineName.data() )
                                                 .setEngineVersion( VK_MAKE_VERSION( 1, 0, 0 ) )
                                                 .setApiVersion( VK_API_VERSION_1_4 );

  inline vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo( {}, &applicationInfo, {}, cfg::InstanceExtensions );

  namespace raii
  {

    struct Allocator
    {
      VmaAllocator allocator = nullptr;

      Allocator( const vk::raii::Instance & instance, const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::Device & device )
      {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_4;
        allocatorInfo.physicalDevice         = *physicalDevice;
        allocatorInfo.device                 = *device;
        allocatorInfo.instance               = *instance;

        vmaCreateAllocator( &allocatorInfo, &allocator );
      }

      operator VmaAllocator() const
      {
        return allocator;
      }

      ~Allocator()
      {
        clear();
      }

      void clear()
      {
        if ( allocator )
        {
          vmaDestroyAllocator( allocator );
          allocator = nullptr;
        }
      }
    };

    struct DepthResources
    {
      VmaAllocator  allocator = nullptr;
      vk::Image     image;
      VmaAllocation allocation;
      vk::Device    device;
      vk::ImageView imageView;
      vk::Format    depthFormat = vk::Format::eD32Sfloat;

      DepthResources( vk::raii::Device & device, VmaAllocator allocator, vk::Extent2D extent )
      {
        this->allocator = allocator;
        this->device    = *device;

        vk::ImageCreateInfo imageInfo;
        imageInfo.setImageType( vk::ImageType::e2D )
          .setExtent( vk::Extent3D{ extent.width, extent.height, 1 } )
          .setMipLevels( 1 )
          .setArrayLayers( 1 )
          .setFormat( depthFormat )
          .setTiling( vk::ImageTiling::eOptimal )
          .setUsage( vk::ImageUsageFlagBits::eDepthStencilAttachment )
          .setSamples( vk::SampleCountFlagBits::e1 )
          .setSharingMode( vk::SharingMode::eExclusive );

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags                   = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        vmaCreateImage(
          allocator, reinterpret_cast<const VkImageCreateInfo *>( &imageInfo ), &allocInfo, reinterpret_cast<VkImage *>( &image ), &allocation, nullptr );

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.setImage( image )
          .setViewType( vk::ImageViewType::e2D )
          .setFormat( depthFormat )
          .setSubresourceRange(
            vk::ImageSubresourceRange{}
              .setAspectMask( vk::ImageAspectFlagBits::eDepth )
              .setBaseMipLevel( 0 )
              .setLevelCount( 1 )
              .setBaseArrayLayer( 0 )
              .setLayerCount( 1 ) );

        imageView = this->device.createImageView( viewInfo );
      }

      // Add move assignment operator
      DepthResources & operator=( DepthResources && other ) noexcept
      {
        if ( this != &other )
        {
          // Clean up current resources
          clear();

          // Move from other
          allocator  = other.allocator;
          device     = other.device;
          image      = other.image;
          allocation = other.allocation;
          imageView  = other.imageView;

          // Reset other
          other.allocator  = nullptr;
          other.device     = VK_NULL_HANDLE;
          other.image      = VK_NULL_HANDLE;
          other.allocation = nullptr;
          other.imageView  = VK_NULL_HANDLE;
        }
        return *this;
      }

      // Delete copy operations
      DepthResources( const DepthResources & )             = delete;
      DepthResources & operator=( const DepthResources & ) = delete;

      ~DepthResources()
      {
        clear();
      }

      void clear()
      {
        if ( allocator )
        {
          if ( imageView )
          {
            device.destroyImageView( imageView );
            imageView = VK_NULL_HANDLE;
          }
          vmaDestroyImage( allocator, image, allocation );
        }
      }
    };

    struct ColorTarget
    {
      VmaAllocator  allocator = nullptr;
      vk::Image     image;
      VmaAllocation allocation;
      vk::Device    device;
      vk::ImageView imageView;
      vk::Format    colorFormat = vk::Format::eUndefined;
      vk::Extent2D  extent{};

      ColorTarget() = default;

      ColorTarget( vk::raii::Device & device, VmaAllocator allocator, vk::Extent2D extent, vk::Format format )
      {
        this->allocator   = allocator;
        this->device      = *device;
        this->extent      = extent;
        this->colorFormat = format;

        vk::ImageCreateInfo imageInfo;
        imageInfo.setImageType( vk::ImageType::e2D )
          .setExtent( vk::Extent3D{ extent.width, extent.height, 1 } )
          .setMipLevels( 1 )
          .setArrayLayers( 1 )
          .setFormat( colorFormat )
          .setTiling( vk::ImageTiling::eOptimal )
          .setUsage( vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc )
          .setSamples( vk::SampleCountFlagBits::e1 )
          .setSharingMode( vk::SharingMode::eExclusive );

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags                   = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        vmaCreateImage(
          allocator, reinterpret_cast<const VkImageCreateInfo *>( &imageInfo ), &allocInfo, reinterpret_cast<VkImage *>( &image ), &allocation, nullptr );

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.setImage( image )
          .setViewType( vk::ImageViewType::e2D )
          .setFormat( colorFormat )
          .setSubresourceRange(
            vk::ImageSubresourceRange{}
              .setAspectMask( vk::ImageAspectFlagBits::eColor )
              .setBaseMipLevel( 0 )
              .setLevelCount( 1 )
              .setBaseArrayLayer( 0 )
              .setLayerCount( 1 ) );

        imageView = this->device.createImageView( viewInfo );
      }

      ColorTarget & operator=( ColorTarget && other ) noexcept
      {
        if ( this != &other )
        {
          clear();
          allocator         = other.allocator;
          device            = other.device;
          image             = other.image;
          allocation        = other.allocation;
          imageView         = other.imageView;
          colorFormat       = other.colorFormat;
          extent            = other.extent;
          other.allocator   = nullptr;
          other.device      = VK_NULL_HANDLE;
          other.image       = VK_NULL_HANDLE;
          other.allocation  = nullptr;
          other.imageView   = VK_NULL_HANDLE;
          other.colorFormat = vk::Format::eUndefined;
          other.extent      = vk::Extent2D{};
        }
        return *this;
      }

      ColorTarget( const ColorTarget & )             = delete;
      ColorTarget & operator=( const ColorTarget & ) = delete;

      ~ColorTarget()
      {
        clear();
      }

      void clear()
      {
        if ( allocator )
        {
          if ( imageView )
          {
            device.destroyImageView( imageView );
            imageView = VK_NULL_HANDLE;
          }
          vmaDestroyImage( allocator, image, allocation );
        }
      }
    };

    struct ShaderBundle
    {
      vk::raii::PipelineLayout         pipelineLayout;
      std::vector<vk::raii::ShaderEXT> vertexShaders;
      std::vector<vk::raii::ShaderEXT> fragmentShaders;

      // Current selected shader indices
      int selectedVertexShader   = 0;
      int selectedFragmentShader = 0;

      // Shader names for ImGui display
      std::vector<std::string> vertexShaderNames;
      std::vector<std::string> fragmentShaderNames;

      ShaderBundle(
        const vk::raii::Device &         device,
        const std::vector<std::string> & vertShaderNames,
        const std::vector<std::string> & fragShaderNames,
        const vk::PushConstantRange &    pushConstantRange = {} )
        : pipelineLayout( createPipelineLayout( device, pushConstantRange ) ), vertexShaderNames( vertShaderNames ), fragmentShaderNames( fragShaderNames )
      {
        // Create vertex shaders
        for ( const auto & shaderName : vertShaderNames )
        {
          vertexShaders.emplace_back( createShader( device, shaderName, vk::ShaderStageFlagBits::eVertex, pushConstantRange ) );
        }

        // Create fragment shaders
        for ( const auto & shaderName : fragShaderNames )
        {
          fragmentShaders.emplace_back( createShader( device, shaderName, vk::ShaderStageFlagBits::eFragment, pushConstantRange ) );
        }
      }

      // Get currently selected vertex shader
      vk::raii::ShaderEXT & getCurrentVertexShader()
      {
        return vertexShaders[selectedVertexShader];
      }

      // Get currently selected fragment shader
      vk::raii::ShaderEXT & getCurrentFragmentShader()
      {
        return fragmentShaders[selectedFragmentShader];
      }

      // Set selected vertex shader by index
      void setVertexShader( int index )
      {
        if ( index >= 0 && index < static_cast<int>( vertexShaders.size() ) )
        {
          selectedVertexShader = index;
        }
      }

      // Set selected fragment shader by index
      void setFragmentShader( int index )
      {
        if ( index >= 0 && index < static_cast<int>( fragmentShaders.size() ) )
        {
          selectedFragmentShader = index;
        }
      }

      // Get shader count for ImGui
      int getVertexShaderCount() const
      {
        return static_cast<int>( vertexShaders.size() );
      }

      int getFragmentShaderCount() const
      {
        return static_cast<int>( fragmentShaders.size() );
      }

    private:
      vk::raii::PipelineLayout createPipelineLayout( const vk::raii::Device & device, const vk::PushConstantRange & pushConstantRange )
      {
        vk::PipelineLayoutCreateInfo layoutInfo{};
        if ( pushConstantRange.size > 0 )
        {
          layoutInfo.setPushConstantRangeCount( 1 ).setPPushConstantRanges( &pushConstantRange );
        }
        return vk::raii::PipelineLayout( device, layoutInfo );
      }

      vk::raii::ShaderEXT createShader(
        const vk::raii::Device & device, const std::string & shaderName, vk::ShaderStageFlagBits stage, const vk::PushConstantRange & pushConstantRange )
      {
        std::vector<uint32_t> shaderCode = core::help::getShaderCode( shaderName );

        vk::ShaderCreateInfoEXT shaderInfo{};
        shaderInfo.setStage( stage )
          .setCodeType( vk::ShaderCodeTypeEXT::eSpirv )
          .setPCode( shaderCode.data() )
          .setPName( "main" )
          .setCodeSize( shaderCode.size() * sizeof( uint32_t ) );

        if ( pushConstantRange.size > 0 )
        {
          shaderInfo.setPushConstantRangeCount( 1 ).setPPushConstantRanges( &pushConstantRange );
        }

        // Set next stage for vertex shaders
        if ( stage == vk::ShaderStageFlagBits::eVertex )
        {
          shaderInfo.setNextStage( vk::ShaderStageFlagBits::eFragment );
        }

        return vk::raii::ShaderEXT( device, shaderInfo );
      }
    };

    struct IMGUI
    {
      vk::raii::DescriptorPool descriptorPool;

      IMGUI(
        vk::raii::Device const &         device,
        vk::raii::Instance const &       instance,
        vk::raii::PhysicalDevice const & physicalDevice,
        uint32_t                         queueFamily,
        vk::raii::Queue const &          queue,
        GLFWwindow *                     window,
        uint32_t                         minImageCount,
        uint32_t                         imageCount,
        vk::Format                       swapchainFormat,
        vk::Format                       depthFormat )
        : descriptorPool( createDescriptorPool( device ) )
      {
        // Initialize ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        // Initialize GLFW implementation
        ImGui_ImplGlfw_InitForVulkan( window, true );

        // Initialize Vulkan implementation
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance            = *instance;
        initInfo.PhysicalDevice      = *physicalDevice;
        initInfo.Device              = *device;
        initInfo.QueueFamily         = queueFamily;
        initInfo.Queue               = *queue;
        initInfo.DescriptorPool      = *descriptorPool;
        initInfo.RenderPass          = VK_NULL_HANDLE;
        initInfo.MinImageCount       = minImageCount;
        initInfo.ImageCount          = imageCount;
        initInfo.MSAASamples         = VK_SAMPLE_COUNT_1_BIT;
        initInfo.UseDynamicRendering = true;

        // Set up dynamic rendering info
        vk::PipelineRenderingCreateInfoKHR pipelineRenderingInfo{};
        pipelineRenderingInfo.colorAttachmentCount    = 1;
        pipelineRenderingInfo.pColorAttachmentFormats = &swapchainFormat;
        pipelineRenderingInfo.depthAttachmentFormat   = depthFormat;

        initInfo.PipelineRenderingCreateInfo = pipelineRenderingInfo;

        ImGui_ImplVulkan_Init( &initInfo );
      }

      ~IMGUI()
      {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        // descriptorPool is automatically destroyed by vk::raii::DescriptorPool
      }

      // Delete copy operations
      IMGUI( IMGUI const & )             = delete;
      IMGUI & operator=( IMGUI const & ) = delete;

      // Allow move operations
      IMGUI( IMGUI && other ) noexcept             = default;
      IMGUI & operator=( IMGUI && other ) noexcept = default;

    private:
      static vk::raii::DescriptorPool createDescriptorPool( vk::raii::Device const & device )
      {
        std::array<vk::DescriptorPoolSize, 11> poolSizes = { vk::DescriptorPoolSize{ vk::DescriptorType::eSampler, 1000 },
                                                             vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 1000 },
                                                             vk::DescriptorPoolSize{ vk::DescriptorType::eSampledImage, 1000 },
                                                             vk::DescriptorPoolSize{ vk::DescriptorType::eStorageImage, 1000 },
                                                             vk::DescriptorPoolSize{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
                                                             vk::DescriptorPoolSize{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
                                                             vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 1000 },
                                                             vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, 1000 },
                                                             vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
                                                             vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
                                                             vk::DescriptorPoolSize{ vk::DescriptorType::eInputAttachment, 1000 } };

        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets       = 1000 * static_cast<uint32_t>( poolSizes.size() );
        poolInfo.poolSizeCount = static_cast<uint32_t>( poolSizes.size() );
        poolInfo.pPoolSizes    = poolSizes.data();

        return vk::raii::DescriptorPool{ device, poolInfo };
      }
    };
  }  // namespace raii

}  // namespace core
