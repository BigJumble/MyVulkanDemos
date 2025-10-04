#include "bootstrap.hpp"

constexpr std::string_view AppName    = "ComputeTextureApp";
constexpr std::string_view EngineName = "MyEngine";

// Helper function to find memory type
uint32_t findMemoryType( vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties )
{
  vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

  for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ )
  {
    if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[i].propertyFlags & properties ) == properties )
    {
      return i;
    }
  }

  throw std::runtime_error( "Failed to find suitable memory type!" );
}

// Structure to hold texture resources
struct TextureResource
{
  vk::raii::Image       image       = nullptr;
  vk::raii::DeviceMemory memory      = nullptr;
  vk::raii::ImageView   imageView   = nullptr;
  vk::raii::Sampler     sampler     = nullptr;
  vk::Extent2D          extent;
};

// Create a texture image for compute shader output
TextureResource createComputeTexture( const vk::raii::Device & device, vk::raii::PhysicalDevice & physicalDevice, vk::Extent2D extent )
{
  TextureResource tex;
  tex.extent = extent;

  // Create image
  vk::ImageCreateInfo imageInfo{};
  imageInfo.setImageType( vk::ImageType::e2D )
    .setFormat( vk::Format::eR8G8B8A8Unorm )
    .setExtent( vk::Extent3D{ extent.width, extent.height, 1 } )
    .setMipLevels( 1 )
    .setArrayLayers( 1 )
    .setSamples( vk::SampleCountFlagBits::e1 )
    .setTiling( vk::ImageTiling::eOptimal )
    .setUsage( vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled )
    .setSharingMode( vk::SharingMode::eExclusive )
    .setInitialLayout( vk::ImageLayout::eUndefined );

  tex.image = vk::raii::Image{ device, imageInfo };

  // Allocate memory for the image
  vk::MemoryRequirements memRequirements = tex.image.getMemoryRequirements();

  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.setAllocationSize( memRequirements.size ).setMemoryTypeIndex(
    findMemoryType( *physicalDevice, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal ) );

  tex.memory = vk::raii::DeviceMemory{ device, allocInfo };
  tex.image.bindMemory( *tex.memory, 0 );

  // Create image view
  vk::ImageViewCreateInfo viewInfo{};
  viewInfo.setImage( *tex.image )
    .setViewType( vk::ImageViewType::e2D )
    .setFormat( vk::Format::eR8G8B8A8Unorm )
    .setSubresourceRange( vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } );

  tex.imageView = vk::raii::ImageView{ device, viewInfo };

  // Create sampler
  vk::SamplerCreateInfo samplerInfo{};
  samplerInfo.setMagFilter( vk::Filter::eNearest )
    .setMinFilter( vk::Filter::eNearest )
    .setMipmapMode( vk::SamplerMipmapMode::eNearest )
    .setAddressModeU( vk::SamplerAddressMode::eRepeat )
    .setAddressModeV( vk::SamplerAddressMode::eRepeat )
    .setAddressModeW( vk::SamplerAddressMode::eRepeat )
    .setAnisotropyEnable( VK_FALSE )
    .setMaxAnisotropy( 1.0f )
    .setBorderColor( vk::BorderColor::eIntOpaqueBlack )
    .setUnnormalizedCoordinates( VK_FALSE )
    .setCompareEnable( VK_FALSE )
    .setCompareOp( vk::CompareOp::eAlways )
    .setMipLodBias( 0.0f )
    .setMinLod( 0.0f )
    .setMaxLod( 0.0f );

  tex.sampler = vk::raii::Sampler{ device, samplerInfo };

  return tex;
}

static void recordComputeCommandBuffer( vk::raii::CommandBuffer & cmd, vk::raii::ShaderEXT & computeShader, vk::raii::PipelineLayout & computePipelineLayout, vk::DescriptorSet descriptorSet, vk::Extent2D extent )
{
  cmd.reset();
  cmd.begin( vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

  // Bind compute shader
  std::array<vk::ShaderStageFlagBits, 1> computeStage = { vk::ShaderStageFlagBits::eCompute };
  std::array<vk::ShaderEXT, 1>           computeShaderArray = { *computeShader };
  cmd.bindShadersEXT( computeStage, computeShaderArray );

  // Bind descriptor set for compute shader
  cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, *computePipelineLayout, 0, { descriptorSet }, {} );

  // Dispatch compute work groups
  // Work group size is 16x16, so calculate how many groups we need
  uint32_t groupCountX = ( extent.width + 15 ) / 16;
  uint32_t groupCountY = ( extent.height + 15 ) / 16;
  cmd.dispatch( groupCountX, groupCountY, 1 );

  cmd.end();
}

static void recordGraphicsCommandBuffer( vk::raii::CommandBuffer & cmd,
                                          vk::raii::ShaderEXT &     vertShaderObject,
                                          vk::raii::ShaderEXT &     fragShaderObject,
                                          vk::raii::PipelineLayout & graphicsPipelineLayout,
                                          core::SwapchainBundle &   swapchainBundle,
                                          uint32_t                  imageIndex,
                                          vk::DescriptorSet         graphicsDescriptorSet )
{
  cmd.reset();
  cmd.begin( vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

  vk::ImageSubresourceRange subresourceRange{};
  subresourceRange.setAspectMask( vk::ImageAspectFlagBits::eColor ).setLevelCount( 1 ).setLayerCount( 1 );

  vk::ImageMemoryBarrier2 barrier{};
  barrier.setSrcStageMask( vk::PipelineStageFlagBits2::eTopOfPipe )
    .setSrcAccessMask( vk::AccessFlagBits2::eNone )
    .setDstStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
    .setDstAccessMask( vk::AccessFlagBits2::eColorAttachmentWrite )
    .setOldLayout( vk::ImageLayout::eUndefined )
    .setNewLayout( vk::ImageLayout::eColorAttachmentOptimal )
    .setSrcQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
    .setDstQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
    .setImage( swapchainBundle.images[imageIndex] )
    .setSubresourceRange( subresourceRange );

  vk::DependencyInfo depInfo{};
  depInfo.setImageMemoryBarrierCount( 1 ).setPImageMemoryBarriers( &barrier );

  cmd.pipelineBarrier2( depInfo );

  vk::ClearValue clearValue{};
  clearValue.color = vk::ClearColorValue( std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f } );

  vk::RenderingAttachmentInfo colorAttachment{};
  colorAttachment.setImageView( swapchainBundle.imageViews[imageIndex] )
    .setImageLayout( vk::ImageLayout::eColorAttachmentOptimal )
    .setLoadOp( vk::AttachmentLoadOp::eClear )
    .setStoreOp( vk::AttachmentStoreOp::eStore )
    .setClearValue( clearValue );

  vk::Rect2D renderArea{};
  renderArea.setExtent( swapchainBundle.extent );

  vk::RenderingInfo renderingInfo{};
  renderingInfo.setRenderArea( renderArea ).setLayerCount( 1 ).setColorAttachmentCount( 1 ).setPColorAttachments( &colorAttachment );

  cmd.beginRendering( renderingInfo );

  std::array<vk::ShaderStageFlagBits, 2> stages  = { vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment };
  std::array<vk::ShaderEXT, 2>           shaders = { *vertShaderObject, *fragShaderObject };
  cmd.bindShadersEXT( stages, shaders );

  // Bind descriptor set for graphics pipeline (texture sampling)
  cmd.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, *graphicsPipelineLayout, 0, { graphicsDescriptorSet }, {} );

  vk::Viewport viewport{ 0, 0, float( swapchainBundle.extent.width ), float( swapchainBundle.extent.height ), 0.0f, 1.0f };
  vk::Rect2D   scissor{ { 0, 0 }, swapchainBundle.extent };
  cmd.setViewportWithCount( viewport );
  cmd.setScissorWithCount( scissor );

  cmd.setVertexInputEXT( {}, {} );
  cmd.setRasterizerDiscardEnable( VK_FALSE );
  cmd.setCullMode( vk::CullModeFlagBits::eNone );
  cmd.setFrontFace( vk::FrontFace::eCounterClockwise );
  cmd.setDepthTestEnable( VK_FALSE );
  cmd.setDepthWriteEnable( VK_FALSE );
  cmd.setDepthCompareOp( vk::CompareOp::eNever );
  cmd.setDepthBiasEnable( VK_FALSE );
  cmd.setStencilTestEnable( VK_FALSE );
  cmd.setPrimitiveTopology( vk::PrimitiveTopology::eTriangleStrip );
  cmd.setPrimitiveRestartEnable( VK_FALSE );
  cmd.setPolygonModeEXT( vk::PolygonMode::eFill );
  cmd.setRasterizationSamplesEXT( vk::SampleCountFlagBits::e1 );

  vk::SampleMask sampleMask = 0xFFFFFFFF;
  cmd.setSampleMaskEXT( vk::SampleCountFlagBits::e1, sampleMask );
  cmd.setAlphaToCoverageEnableEXT( VK_FALSE );
  cmd.setColorBlendEnableEXT( 0, VK_FALSE );
  cmd.setColorBlendEquationEXT( 0, vk::ColorBlendEquationEXT{} );
  vk::ColorComponentFlags colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  cmd.setColorWriteMaskEXT( 0, colorWriteMask );

  cmd.draw( 4, 1, 0, 0 );

  cmd.endRendering();

  barrier.setSrcStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
    .setSrcAccessMask( vk::AccessFlagBits2::eColorAttachmentWrite )
    .setDstStageMask( vk::PipelineStageFlagBits2::eBottomOfPipe )
    .setDstAccessMask( vk::AccessFlagBits2::eNone )
    .setOldLayout( vk::ImageLayout::eColorAttachmentOptimal )
    .setNewLayout( vk::ImageLayout::ePresentSrcKHR );

  depInfo.setImageMemoryBarrierCount( 1 ).setPImageMemoryBarriers( &barrier );

  cmd.pipelineBarrier2( depInfo );

  cmd.end();
}

static void framebufferResizeCallback( GLFWwindow * win, int, int )
{
  auto * resized = static_cast<bool *>( glfwGetWindowUserPointer( win ) );
  if ( resized )
  {
    *resized = true;
  }
}


static void recreateSwapchain(
  core::DisplayBundle &      displayBundle,
  vk::raii::PhysicalDevice & physicalDevice,
  core::DeviceBundle &       deviceBundle,
  core::SwapchainBundle &    swapchainBundle,
  core::QueueFamilyIndices & queueFamilyIndices )
{
  int width = 0, height = 0;
  do
  {
    glfwGetFramebufferSize( displayBundle.window, &width, &height );
    glfwPollEvents();
  } while ( width == 0 || height == 0 );

  deviceBundle.device.waitIdle();

  core::SwapchainBundle old = std::move( swapchainBundle );
  swapchainBundle           = core::createSwapchain(
    physicalDevice,
    deviceBundle.device,
    displayBundle.surface,
    vk::Extent2D{ static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) },
    queueFamilyIndices,
    &old.swapchain );
}

int main()
{
  isDebug( std::println( "LOADING UP COMPUTE-TEXTURE EXAMPLE!\n" ); );
  try
  {
    vk::raii::Context context;

    vk::raii::Instance instance = core::createInstance( context, std::string( AppName ), std::string( EngineName ) );

    vk::raii::PhysicalDevices physicalDevices( instance );

    vk::raii::PhysicalDevice physicalDevice = core::selectPhysicalDevice( physicalDevices );

    core::DisplayBundle displayBundle( instance, "Compute Texture Example", vk::Extent2D( 1280, 720 ) );

    core::QueueFamilyIndices queueFamilyIndices = core::findQueueFamilies( physicalDevice, displayBundle.surface );

    core::DeviceBundle deviceBundle = core::createDeviceWithQueues( physicalDevice, queueFamilyIndices );

    core::SwapchainBundle swapchainBundle = core::createSwapchain( physicalDevice, deviceBundle.device, displayBundle.surface, displayBundle.extent, queueFamilyIndices );

    // Create texture that will be generated by compute shader
    TextureResource texture = createComputeTexture( deviceBundle.device, physicalDevice, vk::Extent2D{ 512, 512 } );

    // Load shaders
    std::vector<uint32_t> vertShaderCode = core::readSpirvFile( "shaders/triangle.vert.spv" );
    std::vector<uint32_t> fragShaderCode = core::readSpirvFile( "shaders/triangle.frag.spv" );
    std::vector<uint32_t> compShaderCode = core::readSpirvFile( "shaders/texture_gen.comp.spv" );

    // Create descriptor set layouts
    // Compute shader: binding 0 = storage image (write)
    vk::DescriptorSetLayoutBinding computeBinding{};
    computeBinding.setBinding( 0 )
      .setDescriptorType( vk::DescriptorType::eStorageImage )
      .setDescriptorCount( 1 )
      .setStageFlags( vk::ShaderStageFlagBits::eCompute );

    vk::DescriptorSetLayoutCreateInfo computeLayoutInfo{};
    computeLayoutInfo.setBindingCount( 1 ).setPBindings( &computeBinding );

    vk::raii::DescriptorSetLayout computeDescriptorSetLayout{ deviceBundle.device, computeLayoutInfo };

    // Graphics shader: binding 0 = combined image sampler (read)
    vk::DescriptorSetLayoutBinding graphicsBinding{};
    graphicsBinding.setBinding( 0 )
      .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
      .setDescriptorCount( 1 )
      .setStageFlags( vk::ShaderStageFlagBits::eFragment );

    vk::DescriptorSetLayoutCreateInfo graphicsLayoutInfo{};
    graphicsLayoutInfo.setBindingCount( 1 ).setPBindings( &graphicsBinding );

    vk::raii::DescriptorSetLayout graphicsDescriptorSetLayout{ deviceBundle.device, graphicsLayoutInfo };

    // Create pipeline layouts from descriptor set layouts
    vk::PipelineLayoutCreateInfo computePipelineLayoutInfo{};
    computePipelineLayoutInfo.setSetLayoutCount( 1 ).setPSetLayouts( &*computeDescriptorSetLayout );

    vk::raii::PipelineLayout computePipelineLayout{ deviceBundle.device, computePipelineLayoutInfo };

    vk::PipelineLayoutCreateInfo graphicsPipelineLayoutInfo{};
    graphicsPipelineLayoutInfo.setSetLayoutCount( 1 ).setPSetLayouts( &*graphicsDescriptorSetLayout );

    vk::raii::PipelineLayout graphicsPipelineLayout{ deviceBundle.device, graphicsPipelineLayoutInfo };

    // Create descriptor pool
    std::array<vk::DescriptorPoolSize, 2> poolSizes = { vk::DescriptorPoolSize{ vk::DescriptorType::eStorageImage, 1 },
                                                         vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 1 } };

    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.setFlags( vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet )
      .setMaxSets( 2 )
      .setPoolSizeCount( poolSizes.size() )
      .setPPoolSizes( poolSizes.data() );

    vk::raii::DescriptorPool descriptorPool{ deviceBundle.device, poolInfo };

    // Allocate descriptor sets
    std::array<vk::DescriptorSetLayout, 1> computeLayouts  = { *computeDescriptorSetLayout };
    std::array<vk::DescriptorSetLayout, 1> graphicsLayouts = { *graphicsDescriptorSetLayout };

    vk::DescriptorSetAllocateInfo computeAllocInfo{};
    computeAllocInfo.setDescriptorPool( *descriptorPool ).setDescriptorSetCount( 1 ).setPSetLayouts( computeLayouts.data() );

    vk::raii::DescriptorSets computeDescriptorSets{ deviceBundle.device, computeAllocInfo };

    vk::DescriptorSetAllocateInfo graphicsAllocInfo{};
    graphicsAllocInfo.setDescriptorPool( *descriptorPool ).setDescriptorSetCount( 1 ).setPSetLayouts( graphicsLayouts.data() );

    vk::raii::DescriptorSets graphicsDescriptorSets{ deviceBundle.device, graphicsAllocInfo };

    // Update descriptor sets
    // Compute: bind texture as storage image
    vk::DescriptorImageInfo computeImageInfo{};
    computeImageInfo.setImageView( *texture.imageView ).setImageLayout( vk::ImageLayout::eGeneral );

    vk::WriteDescriptorSet computeWrite{};
    computeWrite.setDstSet( computeDescriptorSets[0] )
      .setDstBinding( 0 )
      .setDstArrayElement( 0 )
      .setDescriptorType( vk::DescriptorType::eStorageImage )
      .setDescriptorCount( 1 )
      .setPImageInfo( &computeImageInfo );

    // Graphics: bind texture as sampled image
    vk::DescriptorImageInfo graphicsImageInfo{};
    graphicsImageInfo.setImageView( *texture.imageView ).setImageLayout( vk::ImageLayout::eShaderReadOnlyOptimal ).setSampler( *texture.sampler );

    vk::WriteDescriptorSet graphicsWrite{};
    graphicsWrite.setDstSet( graphicsDescriptorSets[0] )
      .setDstBinding( 0 )
      .setDstArrayElement( 0 )
      .setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
      .setDescriptorCount( 1 )
      .setPImageInfo( &graphicsImageInfo );

    deviceBundle.device.updateDescriptorSets( { computeWrite, graphicsWrite }, {} );

    // Create shader objects
    vk::ShaderCreateInfoEXT vertInfo{};
    vertInfo.setStage( vk::ShaderStageFlagBits::eVertex )
      .setCodeType( vk::ShaderCodeTypeEXT::eSpirv )
      .setNextStage( vk::ShaderStageFlagBits::eFragment )
      .setPCode( vertShaderCode.data() )
      .setPName( "main" )
      .setCodeSize( vertShaderCode.size() * sizeof( uint32_t ) )
      .setPSetLayouts( &*graphicsDescriptorSetLayout )
      .setSetLayoutCount( 1 );

    vk::raii::ShaderEXT vertShaderObject{ deviceBundle.device, vertInfo };

    vk::ShaderCreateInfoEXT fragInfo{};
    fragInfo.setStage( vk::ShaderStageFlagBits::eFragment )
      .setCodeType( vk::ShaderCodeTypeEXT::eSpirv )
      .setNextStage( {} )
      .setPCode( fragShaderCode.data() )
      .setPName( "main" )
      .setCodeSize( fragShaderCode.size() * sizeof( uint32_t ) )
      .setPSetLayouts( &*graphicsDescriptorSetLayout )
      .setSetLayoutCount( 1 );

    vk::raii::ShaderEXT fragShaderObject{ deviceBundle.device, fragInfo };

    vk::ShaderCreateInfoEXT compInfo{};
    compInfo.setStage( vk::ShaderStageFlagBits::eCompute )
      .setCodeType( vk::ShaderCodeTypeEXT::eSpirv )
      .setNextStage( {} )
      .setPCode( compShaderCode.data() )
      .setPName( "main" )
      .setCodeSize( compShaderCode.size() * sizeof( uint32_t ) )
      .setPSetLayouts( &*computeDescriptorSetLayout )
      .setSetLayoutCount( 1 );

    vk::raii::ShaderEXT computeShaderObject{ deviceBundle.device, compInfo };

    // Create command pools (one for graphics, one for compute)
    vk::CommandPoolCreateInfo cmdPoolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndices.graphicsFamily.value() };
    vk::raii::CommandPool     commandPool{ deviceBundle.device, cmdPoolInfo };

    // Frames in flight
    constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;

    vk::CommandBufferAllocateInfo cmdInfo{ commandPool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT };
    vk::raii::CommandBuffers      graphicsCmds{ deviceBundle.device, cmdInfo };

    // Single compute command buffer (reused)
    vk::CommandBufferAllocateInfo computeCmdInfo{ commandPool, vk::CommandBufferLevel::ePrimary, 1 };
    vk::raii::CommandBuffers      computeCmds{ deviceBundle.device, computeCmdInfo };

    // Create per-frame synchronization objects
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence>     presentFences;

    imageAvailableSemaphores.reserve( MAX_FRAMES_IN_FLIGHT );
    renderFinishedSemaphores.reserve( MAX_FRAMES_IN_FLIGHT );
    presentFences.reserve( MAX_FRAMES_IN_FLIGHT );

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
      imageAvailableSemaphores.emplace_back( deviceBundle.device, vk::SemaphoreCreateInfo{} );
      renderFinishedSemaphores.emplace_back( deviceBundle.device, vk::SemaphoreCreateInfo{} );
      presentFences.emplace_back( deviceBundle.device, vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled } );
    }

    // One-time fence for compute shader execution
    vk::raii::Fence computeFence{ deviceBundle.device, vk::FenceCreateInfo{} };

    bool framebufferResized = false;
    glfwSetWindowUserPointer( displayBundle.window, &framebufferResized );
    glfwSetFramebufferSizeCallback( displayBundle.window, framebufferResizeCallback );

    // Execute compute shader once to generate texture
    isDebug( std::println( "Generating texture with compute shader..." ); );
    {
      auto & computeCmd = computeCmds[0];

      // Transition image to GENERAL layout for compute write
      computeCmd.begin( vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

      vk::ImageMemoryBarrier2 barrier{};
      barrier.setSrcStageMask( vk::PipelineStageFlagBits2::eTopOfPipe )
        .setSrcAccessMask( vk::AccessFlagBits2::eNone )
        .setDstStageMask( vk::PipelineStageFlagBits2::eComputeShader )
        .setDstAccessMask( vk::AccessFlagBits2::eShaderWrite )
        .setOldLayout( vk::ImageLayout::eUndefined )
        .setNewLayout( vk::ImageLayout::eGeneral )
        .setSrcQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setDstQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setImage( *texture.image )
        .setSubresourceRange( vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } );

      vk::DependencyInfo depInfo{};
      depInfo.setImageMemoryBarrierCount( 1 ).setPImageMemoryBarriers( &barrier );
      computeCmd.pipelineBarrier2( depInfo );

      computeCmd.end();

      // Submit and wait
      vk::CommandBufferSubmitInfo cmdBufferInfo{};
      cmdBufferInfo.setCommandBuffer( *computeCmd );

      vk::SubmitInfo2 submitInfo{};
      submitInfo.setCommandBufferInfoCount( 1 ).setPCommandBufferInfos( &cmdBufferInfo );

      deviceBundle.graphicsQueue.submit2( submitInfo, *computeFence );
      (void)deviceBundle.device.waitForFences( { *computeFence }, VK_TRUE, UINT64_MAX );
      deviceBundle.device.resetFences( { *computeFence } );

      // Now dispatch compute work
      recordComputeCommandBuffer( computeCmd, computeShaderObject, computePipelineLayout, computeDescriptorSets[0], texture.extent );

      cmdBufferInfo.setCommandBuffer( *computeCmd );
      submitInfo.setCommandBufferInfoCount( 1 ).setPCommandBufferInfos( &cmdBufferInfo );

      deviceBundle.graphicsQueue.submit2( submitInfo, *computeFence );
      (void)deviceBundle.device.waitForFences( { *computeFence }, VK_TRUE, UINT64_MAX );
      deviceBundle.device.resetFences( { *computeFence } );

      // Transition to SHADER_READ_ONLY for graphics pipeline
      computeCmd.reset();
      computeCmd.begin( vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

      barrier.setSrcStageMask( vk::PipelineStageFlagBits2::eComputeShader )
        .setSrcAccessMask( vk::AccessFlagBits2::eShaderWrite )
        .setDstStageMask( vk::PipelineStageFlagBits2::eFragmentShader )
        .setDstAccessMask( vk::AccessFlagBits2::eShaderRead )
        .setOldLayout( vk::ImageLayout::eGeneral )
        .setNewLayout( vk::ImageLayout::eShaderReadOnlyOptimal );

      depInfo.setImageMemoryBarrierCount( 1 ).setPImageMemoryBarriers( &barrier );
      computeCmd.pipelineBarrier2( depInfo );

      computeCmd.end();

      cmdBufferInfo.setCommandBuffer( *computeCmd );
      submitInfo.setCommandBufferInfoCount( 1 ).setPCommandBufferInfos( &cmdBufferInfo );

      deviceBundle.graphicsQueue.submit2( submitInfo, *computeFence );
      (void)deviceBundle.device.waitForFences( { *computeFence }, VK_TRUE, UINT64_MAX );
    }

    isDebug( std::println( "Texture generation complete! Starting render loop..." ); );

    size_t currentFrame = 0;

    while ( !glfwWindowShouldClose( displayBundle.window ) )
    {
      glfwPollEvents();

      if ( framebufferResized )
      {
        framebufferResized = false;
        recreateSwapchain( displayBundle, physicalDevice, deviceBundle, swapchainBundle, queueFamilyIndices );
        continue;
      }

      try
      {
        auto & imageAvailable = imageAvailableSemaphores[currentFrame];
        auto & renderFinished = renderFinishedSemaphores[currentFrame];
        auto & presentFence   = presentFences[currentFrame];

        (void)deviceBundle.device.waitForFences( { *presentFence }, VK_TRUE, UINT64_MAX );
        deviceBundle.device.resetFences( { *presentFence } );

        auto acquire = swapchainBundle.swapchain.acquireNextImage( UINT64_MAX, *imageAvailable, nullptr );
        if ( acquire.first == vk::Result::eErrorOutOfDateKHR )
        {
          recreateSwapchain( displayBundle, physicalDevice, deviceBundle, swapchainBundle, queueFamilyIndices );
          continue;
        }
        uint32_t imageIndex = acquire.second;

        auto & cmd = graphicsCmds[currentFrame];
        recordGraphicsCommandBuffer( cmd, vertShaderObject, fragShaderObject, graphicsPipelineLayout, swapchainBundle, imageIndex, graphicsDescriptorSets[0] );

        std::array<vk::SemaphoreSubmitInfo, 1> waitSemaphoreInfos = {
          vk::SemaphoreSubmitInfo{}.setSemaphore( *imageAvailable ).setStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        };

        std::array<vk::SemaphoreSubmitInfo, 1> signalSemaphoreInfos = {
          vk::SemaphoreSubmitInfo{}.setSemaphore( *renderFinished ).setStageMask( vk::PipelineStageFlagBits2::eAllCommands ),
        };

        vk::CommandBufferSubmitInfo cmdBufferInfo{};
        cmdBufferInfo.setCommandBuffer( *cmd );

        vk::SubmitInfo2 submitInfo{};
        submitInfo.setCommandBufferInfoCount( 1 )
          .setPCommandBufferInfos( &cmdBufferInfo )
          .setWaitSemaphoreInfos( waitSemaphoreInfos )
          .setSignalSemaphoreInfos( signalSemaphoreInfos );

        deviceBundle.graphicsQueue.submit2( submitInfo );

        vk::SwapchainPresentFenceInfoKHR presentFenceInfo{};
        presentFenceInfo.setSwapchainCount( 1 ).setPFences( &*presentFence );

        vk::PresentInfoKHR presentInfo{};
        presentInfo.setPNext( &presentFenceInfo )
          .setWaitSemaphoreCount( 1 )
          .setPWaitSemaphores( &*renderFinished )
          .setSwapchainCount( 1 )
          .setPSwapchains( &*swapchainBundle.swapchain )
          .setPImageIndices( &imageIndex );

        auto presentRes = deviceBundle.graphicsQueue.presentKHR( presentInfo );

        if ( presentRes == vk::Result::eSuboptimalKHR || presentRes == vk::Result::eErrorOutOfDateKHR )
        {
          recreateSwapchain( displayBundle, physicalDevice, deviceBundle, swapchainBundle, queueFamilyIndices );
        }

        currentFrame = ( currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
      }
      catch ( std::exception const & err )
      {
        isDebug( std::println( "Frame rendering exception (recreating swapchain): {}", err.what() ) );
        recreateSwapchain( displayBundle, physicalDevice, deviceBundle, swapchainBundle, queueFamilyIndices );
        continue;
      }
    }

    deviceBundle.device.waitIdle();
  }

  catch ( vk::SystemError & err )
  {
    std::println( "vk::SystemError: {}", err.what() );
    exit( -1 );
  }
  catch ( std::exception & err )
  {
    std::println( "vk::exception: {}", err.what() );
    exit( -1 );
  }
  catch ( ... )
  {
    std::println( "unknown error" );
    exit( -1 );
  }

  return 0;
}
