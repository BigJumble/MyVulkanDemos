#pragma once

#include "state.hpp"
#include "structs.hpp"


#include <fstream>
#include <iostream>
#include <limits>
#include <print>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>


#define GLFW_INCLUDE_VULKAN
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

    if ( !( indices.graphicsFamily && indices.presentFamily && indices.computeFamily ) )
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



  // Creates a core::Texture given the image specs. Allocates memory and creates image, image view, and default sampler.
  // Parameters:
  //   device - logical device used for image/image view/sampler creation
  //   allocator - VMA allocator for image allocation
  //   extent - size of the texture
  //   format - vk::Format for the image
  //   usage - usage flags for image creation (e.g., sampled, transfer, etc)
  //   aspectMask - typically vk::ImageAspectFlagBits::eColor
  // Returns:
  //   Populated core::Texture struct
  inline core::Texture createTexture(
    const vk::Device& device,
    VmaAllocator allocator,
    vk::Extent2D extent,
    vk::Format format,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspectMask
  )
  {
    core::Texture texture;
    texture.format = format;
    texture.extent = extent;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = static_cast<VkFormat>(format);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = static_cast<VkImageUsageFlags>(usage);
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkImage vkImage = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    vmaCreateImage(allocator, &imageInfo, &allocInfo, &vkImage, &allocation, nullptr);
    texture.image = vkImage;
    texture.allocation = allocation;

    // Create image view
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.setImage(texture.image)
      .setViewType(vk::ImageViewType::e2D)
      .setFormat(format)
      .setComponents({})
      .setSubresourceRange({ aspectMask, 0, 1, 0, 1 });
    texture.imageView = vk::ImageView(device.createImageView(viewInfo));

    // Create sampler (default settings, linear filtering, repeat)
    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.setMagFilter(vk::Filter::eLinear)
               .setMinFilter(vk::Filter::eLinear)
               .setMipmapMode(vk::SamplerMipmapMode::eLinear)
               .setAddressModeU(vk::SamplerAddressMode::eRepeat)
               .setAddressModeV(vk::SamplerAddressMode::eRepeat)
               .setAddressModeW(vk::SamplerAddressMode::eRepeat)
               .setAnisotropyEnable(VK_FALSE)
               .setMaxAnisotropy(1.0f)
               .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
               .setUnnormalizedCoordinates(VK_FALSE);
    texture.sampler = vk::Sampler(device.createSampler(samplerInfo));

    return texture;
  }

  // Destroys all Vulkan and VMA resources of a core::Texture.
  // Parameters:
  //   device    - logical device for destroying image view and sampler
  //   allocator - VMA allocator for destroying image and memory
  //   texture   - texture object to destroy (its members are reset)
  inline void destroyTexture(
    const vk::Device& device,
    VmaAllocator allocator,
    core::Texture& texture
  )
  {
    if (texture.sampler) {
      device.destroySampler(texture.sampler, nullptr);
      texture.sampler = VK_NULL_HANDLE;
    }

    if (texture.imageView) {
      device.destroyImageView(texture.imageView, nullptr);
      texture.imageView = VK_NULL_HANDLE;
    }

    if (texture.image && texture.allocation) {
      vmaDestroyImage(allocator, texture.image, texture.allocation);
      texture.image = VK_NULL_HANDLE;
      texture.allocation = nullptr;
    }

    texture.format = vk::Format();
    texture.extent = vk::Extent2D();
  }
  namespace raii
  {

    struct Window
    {
      GLFWwindow* window = nullptr;

      // Default constructor
      Window() = default;

      Window(vk::raii::Instance const & instance);

      // Construct from raw GLFWwindow*
      explicit Window(GLFWwindow* w)
        : window(w)
      {}

      // Move constructor
      Window(Window&& other) noexcept
        : window(other.window)
      {
        other.window = nullptr;
      }

      // Move assignment operator
      Window& operator=(Window&& other) noexcept
      {
        if (this != &other)
        {
          reset();
          window = other.window;
          other.window = nullptr;
        }
        return *this;
      }

      // Delete copy constructor and copy assignment
      Window(const Window&) = delete;
      Window& operator=(const Window&) = delete;

      // Destroy window, if owned
      void reset()
      {
        if (window)
        {
          glfwDestroyWindow(window);
          glfwTerminate();
          window = nullptr;
        }
      }

      // Get underlying pointer
      GLFWwindow* get() const { return window; }

      // Convenience conversion
      operator GLFWwindow*() const { return window; }

      ~Window()
      {
        reset();
      }
    };

    struct Allocator
    {
      VmaAllocator allocator = nullptr;

      // Default constructor
      Allocator() = default;

      // Normal constructor
      Allocator( const vk::raii::Instance & instance, const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::Device & device )
      {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_4;
        allocatorInfo.physicalDevice         = *physicalDevice;
        allocatorInfo.device                 = *device;
        allocatorInfo.instance               = *instance;

        vmaCreateAllocator( &allocatorInfo, &allocator );
      }

      // Move constructor
      Allocator( Allocator&& other ) noexcept
      {
        allocator = other.allocator;
        other.allocator = nullptr;
      }

      // Move assignment operator
      Allocator& operator=( Allocator&& other ) noexcept
      {
        if ( this != &other )
        {
          clear();
          allocator = other.allocator;
          other.allocator = nullptr;
        }
        return *this;
      }

      // Delete copy constructor and copy assignment operator
      Allocator(const Allocator&) = delete;
      Allocator& operator=(const Allocator&) = delete;

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

  // ---------------------------------------------------------------------------
  // Swapchain recreation using global singletons
  // ---------------------------------------------------------------------------
  inline void recreateSwapchain(
    vk::raii::Device& device,
    vk::raii::PhysicalDevice& physicalDevice,
    vk::raii::SurfaceKHR& surface,
    core::QueueFamilyIndices& queueFamilyIndices,
    core::SwapchainBundle& swapchainBundle,
    vk::Extent2D& screenSize,
    VmaAllocator allocator,
    core::Texture& depthTexture,
    core::Texture& basicTargetTexture,
    GLFWwindow* window
  )
  {
    int width = 0, height = 0;
    do
    {
      glfwGetFramebufferSize(window, &width, &height);
      glfwPollEvents();
    } while (width == 0 || height == 0);

    device.waitIdle();

    core::SwapchainBundle old = std::move(swapchainBundle);
    swapchainBundle = core::createSwapchain(
      physicalDevice,
      device,
      surface,
      vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) },
      queueFamilyIndices,
      &old.swapchain);

    screenSize = swapchainBundle.extent;

    // Recreate depth and offscreen color targets
    core::destroyTexture(device, allocator, depthTexture);
    depthTexture = core::createTexture(
      device,
      allocator,
      screenSize,
      vk::Format::eD32Sfloat,
      vk::ImageUsageFlagBits::eDepthStencilAttachment,
      vk::ImageAspectFlagBits::eDepth);

    core::destroyTexture(device, allocator, basicTargetTexture);
    basicTargetTexture = core::createTexture(
      device,
      allocator,
      screenSize,
      swapchainBundle.imageFormat,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageAspectFlagBits::eColor);
  }

}  // namespace core
