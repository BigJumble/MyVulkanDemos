#include "bootstrap.hpp"
#include "helper.hpp"
#include "settings.hpp"
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <glm/glm.hpp>
#include <print>

constexpr std::string_view AppName    = "MyApp";
constexpr std::string_view EngineName = "MyEngine";

struct PushConstants
{
  glm::vec2 pos;
};

struct Vertex
{
  glm::vec2 position;
  glm::vec3 color;
};

static void recordCommandBuffer(
  vk::raii::CommandBuffer &  cmd,
  vk::raii::ShaderEXT &      vertShaderObject,
  vk::raii::ShaderEXT &      fragShaderObject,
  core::SwapchainBundle &    swapchainBundle,
  uint32_t                   imageIndex,
  vk::raii::PipelineLayout & pipelineLayout,
  VkBuffer                   vertexBuffer )
{
  cmd.reset();
  cmd.begin( vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

  vk::ImageSubresourceRange subresourceRange{};
  subresourceRange.setAspectMask( vk::ImageAspectFlagBits::eColor ).setLevelCount( 1 ).setLayerCount( 1 );

  vk::ImageMemoryBarrier2 barrier{};
  barrier.setSrcStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
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

  // Set up vertex input state
  vk::VertexInputBindingDescription2EXT bindingDesc{};
  bindingDesc.setBinding( 0 ).setStride( sizeof( Vertex ) ).setInputRate( vk::VertexInputRate::eVertex ).setDivisor( 1 );

  std::array<vk::VertexInputAttributeDescription2EXT, 2> attributeDescs{};
  attributeDescs[0].setLocation( 0 ).setBinding( 0 ).setFormat( vk::Format::eR32G32Sfloat ).setOffset( offsetof( Vertex, position ) );

  attributeDescs[1].setLocation( 1 ).setBinding( 0 ).setFormat( vk::Format::eR32G32B32Sfloat ).setOffset( offsetof( Vertex, color ) );

  cmd.setVertexInputEXT( bindingDesc, attributeDescs );

  // Bind vertex buffer
  vk::DeviceSize offset = 0;
  cmd.bindVertexBuffers( 0, { vertexBuffer }, { offset } );

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

  // Set push constants using sin/cos of the current time
  float t = static_cast<float>( glfwGetTime() );

  cmd.pushConstants<PushConstants>( *pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, { PushConstants{ glm::vec2( std::sin( t ), std::cos( t ) ) } } );

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

  // No need to recreate per-frame semaphores - they're independent of swapchain
}

int main()
{
  /* VULKAN_HPP_KEY_START */
  isDebug( std::println( "LOADING UP CAMERA INSTANCING EXAMPLE!\n" ); );
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

    // Create VMA allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
    allocatorInfo.physicalDevice = *physicalDevice;
    allocatorInfo.device = *deviceBundle.device;
    allocatorInfo.instance = *instance;
    
    VmaAllocator allocator;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    std::vector<uint32_t> vertShaderCode = core::help::getShaderCode( "triangle.vert" );
    std::vector<uint32_t> fragShaderCode = core::help::getShaderCode( "triangle.frag" );

    vk::PushConstantRange pushConstantRange{};
    pushConstantRange.setStageFlags( vk::ShaderStageFlagBits::eVertex ).setSize( sizeof( PushConstants ) ).setOffset( 0 );

    vk::PipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.setPushConstantRangeCount( 1 ).setPPushConstantRanges( &pushConstantRange );

    vk::raii::PipelineLayout pipelineLayout{ deviceBundle.device, layoutInfo };

    vk::ShaderCreateInfoEXT vertInfo{};
    vertInfo.setStage( vk::ShaderStageFlagBits::eVertex )
      .setCodeType( vk::ShaderCodeTypeEXT::eSpirv )
      .setNextStage( vk::ShaderStageFlagBits::eFragment )
      .setPCode( vertShaderCode.data() )
      .setPName( "main" )
      .setCodeSize( vertShaderCode.size() * sizeof( uint32_t ) )
      .setPushConstantRangeCount( 1 )
      .setPPushConstantRanges( &pushConstantRange );

    vk::raii::ShaderEXT vertShaderObject{ deviceBundle.device, vertInfo };

    vk::ShaderCreateInfoEXT fragInfo{};
    fragInfo.setStage( vk::ShaderStageFlagBits::eFragment )
      .setCodeType( vk::ShaderCodeTypeEXT::eSpirv )
      .setNextStage( {} )
      .setPCode( fragShaderCode.data() )
      .setPName( "main" )
      .setCodeSize( fragShaderCode.size() * sizeof( uint32_t ) )
      .setPushConstantRangeCount( 1 )
      .setPPushConstantRanges( &pushConstantRange );

    vk::raii::ShaderEXT fragShaderObject{ deviceBundle.device, fragInfo };

    // Create vertex buffer using VMA
    std::array<Vertex, 3> vertices = { Vertex{ glm::vec2( 0.0f, -0.5f ), glm::vec3( 0.0f, 1.0f, 1.0f ) },
                                       Vertex{ glm::vec2( 0.5f, 0.5f ), glm::vec3( 0.0f, 0.0f, 0.0f ) },
                                       Vertex{ glm::vec2( -0.5f, 0.5f ), glm::vec3( 0.0f, 0.0f, 0.0f ) } };

    VkDeviceSize bufferSize = sizeof( vertices );

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferAllocation;
    VmaAllocationInfo vertexBufferAllocInfo;

    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &vertexBuffer, &vertexBufferAllocation, &vertexBufferAllocInfo);

    // Copy vertex data to buffer (already mapped)
    memcpy( vertexBufferAllocInfo.pMappedData, vertices.data(), static_cast<size_t>( bufferSize ) );

    vk::CommandPoolCreateInfo cmdPoolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndices.graphicsFamily.value() };
    vk::raii::CommandPool     commandPool{ deviceBundle.device, cmdPoolInfo };

    // Frames in flight - independent of swapchain image count
    constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    vk::CommandBufferAllocateInfo cmdInfo{ commandPool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT };
    vk::raii::CommandBuffers      cmds{ deviceBundle.device, cmdInfo };

    // Create per-frame binary semaphores for image acquisition and presentation
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

    bool framebufferResized = false;
    // Set a pointer to framebufferResized so the resize callback can modify it
    glfwSetWindowUserPointer( displayBundle.window, &framebufferResized );
    // Register a callback function to be called when the window is resized
    glfwSetFramebufferSizeCallback( displayBundle.window, framebufferResizeCallback );

    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
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
        // Get synchronization objects for current frame in flight
        auto & imageAvailable = imageAvailableSemaphores[currentFrame];
        auto & renderFinished = renderFinishedSemaphores[currentFrame];
        auto & presentFence   = presentFences[currentFrame];

        // Wait for the presentation fence from previous use of this frame slot
        // Wait for the present fence to be signaled before reusing this frame slot
        (void)deviceBundle.device.waitForFences( { *presentFence }, VK_TRUE, UINT64_MAX );

        // Acquire next swapchain image using the imageAvailable semaphore
        auto acquire = swapchainBundle.swapchain.acquireNextImage( UINT64_MAX, *imageAvailable, nullptr );
        if ( acquire.result == vk::Result::eErrorOutOfDateKHR )
        {
          throw acquire.result;
        }
        uint32_t imageIndex = acquire.value;

        // Only reset the fence after successful image acquisition to prevent deadlock on exception
        deviceBundle.device.resetFences( { *presentFence } );
        // std::println( "imageIndex: {}", imageIndex );
        // Record command buffer for this frame
        auto & cmd = cmds[currentFrame];
        recordCommandBuffer( cmd, vertShaderObject, fragShaderObject, swapchainBundle, imageIndex, pipelineLayout, vertexBuffer );

        // Submit command buffer waiting on imageAvailable, signal renderFinished and timeline
        // uint64_t renderCompleteValue = ++currentTimelineValue;

        std::array<vk::SemaphoreSubmitInfo, 1> waitSemaphoreInfos = {
          vk::SemaphoreSubmitInfo{}.setSemaphore( *imageAvailable ).setStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        };

        std::array<vk::SemaphoreSubmitInfo, 1> signalSemaphoreInfos = {
          vk::SemaphoreSubmitInfo{}.setSemaphore( *renderFinished ).setStageMask( vk::PipelineStageFlagBits2::eAllCommands ),
          // vk::SemaphoreSubmitInfo{}.setSemaphore( *syncSemaphore ).setValue( renderCompleteValue ).setStageMask( vk::PipelineStageFlagBits2::eAllCommands )
        };

        vk::CommandBufferSubmitInfo cmdBufferInfo{};
        cmdBufferInfo.setCommandBuffer( *cmd );

        vk::SubmitInfo2 submitInfo{};
        submitInfo.setCommandBufferInfoCount( 1 )
          .setPCommandBufferInfos( &cmdBufferInfo )
          .setWaitSemaphoreInfos( waitSemaphoreInfos )
          .setSignalSemaphoreInfos( signalSemaphoreInfos );

        deviceBundle.graphicsQueue.submit2( submitInfo );

        vk::SwapchainPresentFenceInfoEXT presentFenceInfo{};
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
          throw presentRes;
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
    
    // Cleanup VMA resources
    vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferAllocation);
    vmaDestroyAllocator(allocator);
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
