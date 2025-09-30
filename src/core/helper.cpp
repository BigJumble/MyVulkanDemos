#include "helper.hpp"

#include "settings.hpp"

// #include <vulkan/vulkan_raii.hpp>`
#include <fstream>
#include <print>
#include <vulkan/vulkan.hpp>

namespace core
{
  VKAPI_ATTR vk::Bool32 VKAPI_CALL debugUtilsMessengerCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT              messageTypes,
    const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData,
    void * /*pUserData*/ )
  {
    std::println( "validation layer (severity: {}): {}\n", vk::to_string( messageSeverity ), pCallbackData->pMessage );
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
      VK_API_VERSION_1_4 );
    // applicationInfo.sType = vk::StructureType::eApplicationInfo;

    // applicationInfo.sType = vk::StructureType::eApplicationInfo;
    vk::InstanceCreateInfo instanceCreateInfo( {}, &applicationInfo, layers, extensions );
    // instanceCreateInfo.sType = vk::StructureType::eInstanceCreateInfo;
// #if defined(DEBUG) || !defined(NDEBUG)
//     vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = core::createDebugUtilsMessengerCreateInfo();
//     instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfo;
// #else
//     instanceCreateInfo.pNext = nullptr;
// #endif
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

    // Enable dynamic rendering feature
    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature{};
    dynamicRenderingFeature.sType = vk::StructureType::ePhysicalDeviceDynamicRenderingFeatures;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    // Test if deviceExtensions are supported
    {
      std::vector<vk::ExtensionProperties> availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
      std::set<std::string> availableExtensionNames;
      for (const auto& ext : availableExtensions) {
        availableExtensionNames.insert(ext.extensionName);
        isDebug(std::println("Device extension: {}", static_cast<const char*>(ext.extensionName)));
      }
      for (const char* requiredExt : core::deviceExtensions) {
        if (availableExtensionNames.find(requiredExt) == availableExtensionNames.end()) {
          throw std::runtime_error(std::string("Device does not support required extension: ") + requiredExt);
        }
      }
    }

    vk::DeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.setQueueCreateInfos( queueCreateInfos );
    deviceCreateInfo.setPEnabledExtensionNames( core::deviceExtensions );
    deviceCreateInfo.pNext = &dynamicRenderingFeature;

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
    createInfo.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage;

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
      vk::ImageViewCreateInfo viewInfo{}; // Create an image view create info struct
      viewInfo.image = image; // The image to create a view for
      viewInfo.viewType = vk::ImageViewType::e2D; // 2D texture view
      viewInfo.format = bundle.imageFormat; // Format matches swapchain image format
      viewInfo.components.r = vk::ComponentSwizzle::eIdentity; // No swizzle for R
      viewInfo.components.g = vk::ComponentSwizzle::eIdentity; // No swizzle for G
      viewInfo.components.b = vk::ComponentSwizzle::eIdentity; // No swizzle for B
      viewInfo.components.a = vk::ComponentSwizzle::eIdentity; // No swizzle for A
      viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor; // View color aspect
      viewInfo.subresourceRange.baseMipLevel = 0; // Start at first mip level
      viewInfo.subresourceRange.levelCount = 1; // Only one mip level
      viewInfo.subresourceRange.baseArrayLayer = 0; // Start at first array layer
      viewInfo.subresourceRange.layerCount = 1; // Only one array layer
      bundle.imageViews.emplace_back( device, viewInfo ); // Create and store the image view
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


  vk::raii::PipelineLayout createPipelineLayout( const vk::raii::Device & device )
  {
    vk::PipelineLayoutCreateInfo info{};
    return vk::raii::PipelineLayout( device, info );
  }

  vk::raii::Pipeline createGraphicsPipeline(
    const vk::raii::Device &         device,
    vk::raii::PipelineLayout const & pipelineLayout,
    vk::Extent2D                     extent,
    vk::raii::ShaderModule const &   vert,
    vk::raii::ShaderModule const &   frag,
    vk::Format                       colorFormat )
  {
    vk::PipelineShaderStageCreateInfo vertStage{};
    vertStage.stage  = vk::ShaderStageFlagBits::eVertex;
    vertStage.module = *vert;
    vertStage.pName  = "main";

    vk::PipelineShaderStageCreateInfo fragStage{};
    fragStage.stage  = vk::ShaderStageFlagBits::eFragment;
    fragStage.module = *frag;
    fragStage.pName  = "main";

    vk::PipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

    vk::PipelineVertexInputStateCreateInfo vertexInput{};  // no vertex buffers

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.topology               = vk::PrimitiveTopology::eTriangleStrip;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>( extent.width );
    viewport.height   = static_cast<float>( extent.height );
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{ 0, 0 };
    scissor.extent = extent;

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.pViewports    = nullptr;  // dynamic
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = nullptr;  // dynamic

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = vk::PolygonMode::eFill;
    rasterizer.cullMode                = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace               = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.lineWidth               = 1.0f;

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable    = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    // Enable dynamic viewport and scissor to avoid pipeline rebuilds on resize
    std::array<vk::DynamicState, 2> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>( dynamicStates.size() );
    dynamicStateInfo.pDynamicStates    = dynamicStates.data();

    // Dynamic rendering info
    vk::PipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = vk::StructureType::ePipelineRenderingCreateInfo;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = stages;
    pipelineInfo.pVertexInputState   = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicStateInfo;
    pipelineInfo.layout              = *pipelineLayout;
    pipelineInfo.renderPass          = VK_NULL_HANDLE; // Must be null for dynamic rendering
    pipelineInfo.pNext               = &renderingInfo;

    return vk::raii::Pipeline( device, nullptr, pipelineInfo );
  }


  CommandResources createCommandResources( const vk::raii::Device & device, uint32_t graphicsQueueFamilyIndex, size_t count )
  {
    CommandResources          res{};
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    res.pool                  = vk::raii::CommandPool( device, poolInfo );

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool        = *res.pool;
    allocInfo.level              = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>( count );
    res.buffers                  = vk::raii::CommandBuffers( device, allocInfo );
    return res;
  }

  void recordTriangleCommands(
    std::vector<vk::raii::CommandBuffer> const & commandBuffers,
    std::vector<vk::raii::ImageView> const &     imageViews,
    vk::Extent2D                                 extent,
    vk::raii::Pipeline const &                   pipeline )
  {
    // Safety check
    if ( commandBuffers.size() != imageViews.size() )
    {
      throw std::runtime_error( "Command buffer count does not match image view count" );
    }

    for ( size_t i = 0; i < commandBuffers.size(); ++i )
    {
      auto & cb = commandBuffers[i];
      cb.begin( vk::CommandBufferBeginInfo{} );

      // Begin dynamic rendering
      vk::ClearValue clearColor = vk::ClearColorValue{ std::array<float, 4>{ 0.02f, 0.02f, 0.03f, 1.0f } };
      
      vk::RenderingAttachmentInfo colorAttachment{};
      colorAttachment.sType = vk::StructureType::eRenderingAttachmentInfo;
      colorAttachment.imageView   = *imageViews[i];
      colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
      colorAttachment.loadOp      = vk::AttachmentLoadOp::eClear;
      colorAttachment.storeOp     = vk::AttachmentStoreOp::eStore;
      colorAttachment.clearValue  = clearColor;

      vk::RenderingInfo renderingInfo{};
      renderingInfo.sType = vk::StructureType::eRenderingInfo;
      renderingInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
      renderingInfo.renderArea.extent = extent;
      renderingInfo.layerCount        = 1;
      renderingInfo.colorAttachmentCount = 1;
      renderingInfo.pColorAttachments    = &colorAttachment;

      // Use the standard beginRendering function (Vulkan 1.3+) instead of KHR extension
      cb.beginRendering( renderingInfo );
      cb.bindPipeline( vk::PipelineBindPoint::eGraphics, *pipeline );
      
      // Set dynamic viewport and scissor to current extent
      vk::Viewport vp{};
      vp.x        = 0.0f;
      vp.y        = 0.0f;
      vp.width    = static_cast<float>( extent.width );
      vp.height   = static_cast<float>( extent.height );
      vp.minDepth = 0.0f;
      vp.maxDepth = 1.0f;
      std::array<vk::Viewport, 1> viewports{ vp };
      cb.setViewport( 0, viewports );

      vk::Rect2D sc{};
      sc.offset = vk::Offset2D{ 0, 0 };
      sc.extent = extent;
      std::array<vk::Rect2D, 1> scissors{ sc };
      cb.setScissor( 0, scissors );
      
      cb.draw( 3, 1, 0, 0 );
      cb.endRendering();
      cb.end();
    }
  }

  SyncObjects createSyncObjects( const vk::raii::Device & device, size_t framesInFlight )
  {
    SyncObjects s{};
    s.imageAvailable.reserve( framesInFlight );
    s.renderFinished.reserve( framesInFlight );
    s.inFlightFences.reserve( framesInFlight );
    vk::SemaphoreCreateInfo semInfo{};
    vk::FenceCreateInfo     fenceInfo{ vk::FenceCreateFlagBits::eSignaled };
    for ( size_t i = 0; i < framesInFlight; ++i )
    {
      s.imageAvailable.emplace_back( device, semInfo );
      s.renderFinished.emplace_back( device, semInfo );
      s.inFlightFences.emplace_back( device, fenceInfo );
    }
    return s;
  }

  uint32_t drawFrame(
    const vk::raii::Device &                     device,
    const vk::raii::SwapchainKHR &               swapchain,
    vk::raii::Queue const &                      graphicsQueue,
    vk::raii::Queue const &                      presentQueue,
    std::vector<vk::raii::CommandBuffer> const & commandBuffers,
    SyncObjects &                                sync,
    size_t &                                     currentFrame )
  {
    (void)device.waitForFences( *sync.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX );

    uint32_t imageIndex = 0;
    auto     acq        = swapchain.acquireNextImage( UINT64_MAX, *sync.imageAvailable[currentFrame], nullptr );
    if ( acq.first != vk::Result::eSuccess && acq.first != vk::Result::eSuboptimalKHR )
    {
      throw std::runtime_error( "Failed to acquire swapchain image" );
    }
    imageIndex = acq.second;

    device.resetFences( *sync.inFlightFences[currentFrame] );

    vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo         submitInfo{};
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &*sync.imageAvailable[currentFrame];
    submitInfo.pWaitDstStageMask    = &waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &*commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &*sync.renderFinished[currentFrame];

    graphicsQueue.submit( submitInfo, *sync.inFlightFences[currentFrame] );

    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &*sync.renderFinished[currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &*swapchain;
    presentInfo.pImageIndices      = &imageIndex;

    vk::Result pres = presentQueue.presentKHR( presentInfo );

    currentFrame = ( currentFrame + 1 ) % sync.imageAvailable.size();
    return imageIndex;
  }

}  // namespace core
