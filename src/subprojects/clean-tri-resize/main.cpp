#include "bootstrap.hpp"

constexpr std::string_view AppName    = "MyApp";
constexpr std::string_view EngineName = "MyEngine";

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

  // Timeline semaphores don't need per-swapchain-image tracking
}

int main()
{
  /* VULKAN_HPP_KEY_START */
  isDebug( std::println( "LOADING UP CLEAR-TRI-RESIZE EXAMPLE!\n" ); );
  try
  {
    vk::raii::Context context;

    vk::raii::Instance instance = core::createInstance( context, std::string( AppName ), std::string( EngineName ) );

    vk::raii::PhysicalDevices physicalDevices( instance );

    vk::raii::PhysicalDevice physicalDevice = core::selectPhysicalDevice( physicalDevices );

    core::DisplayBundle displayBundle( instance, "MyEngine", vk::Extent2D( 1280, 720 ) );

    core::QueueFamilyIndices queueFamilyIndices = core::findQueueFamilies( physicalDevice, displayBundle.surface );

    core::DeviceBundle deviceBundle = core::createDeviceWithQueues( physicalDevice, queueFamilyIndices );

    core::SwapchainBundle swapchainBundle = core::createSwapchain( physicalDevice, deviceBundle.device, displayBundle.surface, displayBundle.extent, queueFamilyIndices );

    std::vector<uint32_t> vertShaderCode = core::readSpirvFile( "shaders/triangle.vert.spv" );
    std::vector<uint32_t> fragShaderCode = core::readSpirvFile( "shaders/triangle.frag.spv" );

    vk::ShaderCreateInfoEXT vertInfo{};
    vertInfo.setStage( vk::ShaderStageFlagBits::eVertex )
      .setCodeType( vk::ShaderCodeTypeEXT::eSpirv )
      .setNextStage( vk::ShaderStageFlagBits::eFragment )
      .setPCode( vertShaderCode.data() )
      .setPName( "main" )
      .setCodeSize( vertShaderCode.size() * sizeof( uint32_t ) );

    vk::raii::ShaderEXT vertShaderObject{ deviceBundle.device, vertInfo };

    vk::ShaderCreateInfoEXT fragInfo{};
    fragInfo.setStage( vk::ShaderStageFlagBits::eFragment )
      .setCodeType( vk::ShaderCodeTypeEXT::eSpirv )
      .setNextStage( {} )
      .setPCode( fragShaderCode.data() )
      .setPName( "main" )
      .setCodeSize( fragShaderCode.size() * sizeof( uint32_t ) );

    vk::raii::ShaderEXT fragShaderObject{ deviceBundle.device, fragInfo };

    vk::CommandPoolCreateInfo cmdPoolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndices.graphicsFamily.value() };
    vk::raii::CommandPool     commandPool{ deviceBundle.device, cmdPoolInfo };

    // Frames in flight - independent of swapchain image count
    constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    vk::CommandBufferAllocateInfo cmdInfo{ commandPool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT };
    vk::raii::CommandBuffers      cmds{ deviceBundle.device, cmdInfo };

    // Synchronization objects using timeline semaphores:
    // - imageAvailableSemaphores: binary semaphore per swapchain image for acquireNextImage (required by spec)
    // - renderFinishedSemaphores: binary semaphore per swapchain image for GPU-GPU sync between render and present
    // - frameSemaphores: timeline semaphores per frame-in-flight for CPU-GPU sync
    // - frameTimelineValues: current timeline value per frame for tracking completion
    vk::SemaphoreCreateInfo binarySemaphoreInfo{};

    // Binary semaphores must be per swapchain image to avoid reuse issues
    size_t swapchainImageCount = swapchainBundle.images.size();
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Semaphore> frameSemaphores;
    std::vector<uint64_t>            frameTimelineValues( MAX_FRAMES_IN_FLIGHT, 0 );

    imageAvailableSemaphores.reserve( swapchainImageCount );
    renderFinishedSemaphores.reserve( swapchainImageCount );
    frameSemaphores.reserve( MAX_FRAMES_IN_FLIGHT );

    vk::SemaphoreTypeCreateInfo timelineCreateInfo{};
    timelineCreateInfo.setSemaphoreType( vk::SemaphoreType::eTimeline ).setInitialValue( 0 );

    vk::SemaphoreCreateInfo timelineSemaphoreInfo{};
    timelineSemaphoreInfo.setPNext( &timelineCreateInfo );

    // Create binary semaphores for each swapchain image
    for ( size_t i = 0; i < swapchainImageCount; ++i )
    {
      imageAvailableSemaphores.emplace_back( deviceBundle.device, binarySemaphoreInfo );
      renderFinishedSemaphores.emplace_back( deviceBundle.device, binarySemaphoreInfo );
    }

    // Create timeline semaphores for each frame-in-flight
    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
      frameSemaphores.emplace_back( deviceBundle.device, timelineSemaphoreInfo );
    }

    bool framebufferResized = false;
    // Set a pointer to framebufferResized so the resize callback can modify it
    glfwSetWindowUserPointer( displayBundle.window, &framebufferResized );
    // Register a callback function to be called when the window is resized
    glfwSetFramebufferSizeCallback( displayBundle.window, framebufferResizeCallback );

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
        // Wait for the current frame-in-flight to finish before reusing its resources
        // Only wait if this frame has been submitted before (timeline value > 0)
        if ( frameTimelineValues[currentFrame] > 0 )
        {
          vk::SemaphoreWaitInfo waitInfo{};
          waitInfo.setSemaphoreCount( 1 ).setPSemaphores( &*frameSemaphores[currentFrame] ).setPValues( &frameTimelineValues[currentFrame] );

          (void)deviceBundle.device.waitSemaphores( waitInfo, UINT64_MAX );
        }

        // Acquire next swapchain image
        // Use a rotating semaphore for acquire that's separate from the image-specific sync
        uint32_t acquireSemaphoreIndex = currentFrame % swapchainImageCount;
        auto     acquire               = swapchainBundle.swapchain.acquireNextImage( UINT64_MAX, *imageAvailableSemaphores[acquireSemaphoreIndex], nullptr );
        if ( acquire.first == vk::Result::eErrorOutOfDateKHR || acquire.first == vk::Result::eSuboptimalKHR )
        {
          recreateSwapchain( displayBundle, physicalDevice, deviceBundle, swapchainBundle, queueFamilyIndices );
          continue;
        }
        uint32_t imageIndex = acquire.second;
        std::println( "imageIndex: {}", imageIndex );
        // Record command buffer for this frame (use currentFrame, not imageIndex)
        auto & cmd = cmds[currentFrame];
        cmd.reset();
        cmd.begin( vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );  // No flags needed - one-time submit per frame

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
        cmd.setPrimitiveTopology( vk::PrimitiveTopology::eTriangleList );
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

        cmd.draw( 3, 1, 0, 0 );

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

        // Increment timeline value for this frame
        uint64_t signalValue = ++frameTimelineValues[currentFrame];

        // Submit with wait on the acquired image semaphore and signal both image-specific render finished and frame timeline
        std::array<vk::SemaphoreSubmitInfo, 1> waitSemaphoreInfos = {
          vk::SemaphoreSubmitInfo{}.setSemaphore( *imageAvailableSemaphores[acquireSemaphoreIndex] ).setStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        };

        // Use image-indexed renderFinished semaphore to avoid reuse conflicts with swapchain
        std::array<vk::SemaphoreSubmitInfo, 2> signalSemaphoreInfos = {
          vk::SemaphoreSubmitInfo{}.setSemaphore( *renderFinishedSemaphores[imageIndex] ).setStageMask( vk::PipelineStageFlagBits2::eAllCommands ),
          vk::SemaphoreSubmitInfo{}.setSemaphore( *frameSemaphores[currentFrame] ).setValue( signalValue ).setStageMask( vk::PipelineStageFlagBits2::eAllCommands )
        };

        vk::CommandBufferSubmitInfo cmdBufferInfo{};
        cmdBufferInfo.setCommandBuffer( *cmd );

        vk::SubmitInfo2 submitInfo{};
        submitInfo.setWaitSemaphoreInfos( waitSemaphoreInfos )
          .setCommandBufferInfoCount( 1 )
          .setPCommandBufferInfos( &cmdBufferInfo )
          .setSignalSemaphoreInfos( signalSemaphoreInfos );

        deviceBundle.graphicsQueue.submit2( submitInfo );

        // Present waits for render to finish (GPU-GPU sync, no CPU stall)
        // Use image-indexed semaphore to match the submit signal
        vk::PresentInfoKHR presentInfo{};
        presentInfo.setWaitSemaphoreCount( 1 )
          .setPWaitSemaphores( &*renderFinishedSemaphores[imageIndex] )
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

  /* VULKAN_HPP_KEY_END */

  return 0;
}
