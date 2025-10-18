#include "bootstrap.hpp"
#include "settings.hpp"

#include <cstddef>
#include <cstdint>
#include <print>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>

static std::string AppName    = "MyApp";
static std::string EngineName = "MyEngine";

int main()
{
  /* VULKAN_HPP_KEY_START */
  isDebug( std::println( "LOADING UP CLEAR-TRI EXAMPLE!\n" ); );
  try
  {
    vk::raii::Context context;

    vk::raii::Instance instance = core::createInstance( context, AppName, EngineName );

    vk::raii::PhysicalDevices physicalDevices( instance );

    vk::raii::PhysicalDevice physicalDevice = core::selectPhysicalDevice( physicalDevices );

    core::DisplayBundle displayBundle( instance, "MyEngine", vk::Extent2D( 1280, 720 ) );

    core::QueueFamilyIndices queueFamilyIndices = core::findQueueFamilies( physicalDevice, displayBundle.surface );

    core::DeviceBundle deviceBundle = core::createDeviceWithQueues( physicalDevice, queueFamilyIndices );

    core::SwapchainBundle swapchainBundle = core::createSwapchain( physicalDevice, deviceBundle.device, displayBundle.surface, displayBundle.extent, queueFamilyIndices );

    std::vector<uint32_t> vertShaderCode = core::readSpirvFile( "shaders/triangle.vert.spv" );
    std::vector<uint32_t> fragShaderCode = core::readSpirvFile( "shaders/triangle.frag.spv" );

    vk::ShaderCreateInfoEXT vertInfo{};
    vertInfo.stage     = vk::ShaderStageFlagBits::eVertex;
    vertInfo.codeType  = vk::ShaderCodeTypeEXT::eSpirv;
    vertInfo.nextStage = vk::ShaderStageFlagBits::eFragment;
    vertInfo.pCode     = vertShaderCode.data();
    vertInfo.pName     = "main";
    vertInfo.codeSize  = vertShaderCode.size() * sizeof( uint32_t );

    vk::raii::ShaderEXT vertShaderObject{ deviceBundle.device, vertInfo };

    vk::ShaderCreateInfoEXT fragInfo{};
    fragInfo.stage     = vk::ShaderStageFlagBits::eFragment;
    fragInfo.codeType  = vk::ShaderCodeTypeEXT::eSpirv;
    fragInfo.nextStage = {};
    fragInfo.pCode     = fragShaderCode.data();
    fragInfo.pName     = "main";
    fragInfo.codeSize  = fragShaderCode.size() * sizeof( uint32_t );

    vk::raii::ShaderEXT fragShaderObject{ deviceBundle.device, fragInfo };

    vk::CommandPoolCreateInfo cmdPoolInfo{ {}, queueFamilyIndices.graphicsFamily.value() };
    vk::raii::CommandPool     commandPool{ deviceBundle.device, cmdPoolInfo };

    vk::CommandBufferAllocateInfo cmdInfo{ commandPool, vk::CommandBufferLevel::ePrimary, (uint32_t)swapchainBundle.images.size() };
    vk::raii::CommandBuffers      cmds{ deviceBundle.device, cmdInfo };

    // Record command buffers, one per swapchain image
    for ( uint32_t i = 0; i < cmds.size(); ++i )
    {
      auto & cmd = cmds[i];
      cmd.begin( vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse } );

      // Transition image to color attachment optimal
      vk::ImageMemoryBarrier barrier{};
      barrier.oldLayout                   = vk::ImageLayout::eUndefined;
      barrier.newLayout                   = vk::ImageLayout::eColorAttachmentOptimal;
      barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
      barrier.image                       = swapchainBundle.images[i];
      barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
      barrier.subresourceRange.levelCount = 1;
      barrier.subresourceRange.layerCount = 1;
      barrier.srcAccessMask               = {};
      barrier.dstAccessMask               = vk::AccessFlagBits::eColorAttachmentWrite;

      cmd.pipelineBarrier( vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {}, barrier );

      vk::RenderingAttachmentInfo colorAttachment{};
      colorAttachment.imageView        = swapchainBundle.imageViews[i];
      colorAttachment.imageLayout      = vk::ImageLayout::eColorAttachmentOptimal;
      colorAttachment.loadOp           = vk::AttachmentLoadOp::eClear;
      colorAttachment.storeOp          = vk::AttachmentStoreOp::eStore;
      colorAttachment.clearValue.color = vk::ClearColorValue( std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f } );

      vk::RenderingInfo renderingInfo{};
      renderingInfo.renderArea.extent    = swapchainBundle.extent;
      renderingInfo.layerCount           = 1;
      renderingInfo.colorAttachmentCount = 1;
      renderingInfo.pColorAttachments    = &colorAttachment;

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

      // Transition to present layout - THIS WAS MISSING
      barrier.oldLayout     = vk::ImageLayout::eColorAttachmentOptimal;
      barrier.newLayout     = vk::ImageLayout::ePresentSrcKHR;
      barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
      barrier.dstAccessMask = {};

      cmd.pipelineBarrier( vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, barrier );

      cmd.end();
    }

    // Synchronization objects - use one set per frame-in-flight
    const size_t                     imageCount = swapchainBundle.images.size();
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence>     inFlightFences;

    imageAvailableSemaphores.reserve( imageCount );
    renderFinishedSemaphores.reserve( imageCount );
    inFlightFences.reserve( imageCount );

    vk::SemaphoreCreateInfo semaphoreInfo{};
    vk::FenceCreateInfo     fenceInfo{ vk::FenceCreateFlagBits::eSignaled };
    for ( size_t i = 0; i < imageCount; ++i )
    {
      imageAvailableSemaphores.emplace_back( deviceBundle.device, semaphoreInfo );
      renderFinishedSemaphores.emplace_back( deviceBundle.device, semaphoreInfo );
      inFlightFences.emplace_back( deviceBundle.device, fenceInfo );
    }

    size_t currentFrame = 0;
    while ( !glfwWindowShouldClose( displayBundle.window ) )
    {
      glfwPollEvents();

      // Ensure the previous work for this frame has completed before reusing its sync objects
      (void)deviceBundle.device.waitForFences( { *inFlightFences[currentFrame] }, VK_TRUE, UINT64_MAX );
      deviceBundle.device.resetFences( { *inFlightFences[currentFrame] } );

      uint32_t imageIndex = 0;
      try
      {
        // Acquire using the semaphore for this frame-in-flight
        auto acquire = swapchainBundle.swapchain.acquireNextImage( UINT64_MAX, *imageAvailableSemaphores[currentFrame], nullptr );
        if ( acquire.result == vk::Result::eErrorOutOfDateKHR )
        {
          break;
        }
        imageIndex = acquire.value;
      }
      catch ( vk::OutOfDateKHRError const & )
      {
        break;
      }
      vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      vk::SubmitInfo         submitInfo{};
      submitInfo.waitSemaphoreCount   = 1;
      submitInfo.pWaitSemaphores      = &*imageAvailableSemaphores[currentFrame];
      submitInfo.pWaitDstStageMask    = &waitStages;
      submitInfo.commandBufferCount   = 1;
      submitInfo.pCommandBuffers      = &*cmds[imageIndex];
      submitInfo.signalSemaphoreCount = 1;
      submitInfo.pSignalSemaphores    = &*renderFinishedSemaphores[currentFrame];

      deviceBundle.graphicsQueue.submit( submitInfo, *inFlightFences[currentFrame] );

      vk::PresentInfoKHR presentInfo{};
      presentInfo.waitSemaphoreCount = 1;
      presentInfo.pWaitSemaphores    = &*renderFinishedSemaphores[currentFrame];
      presentInfo.swapchainCount     = 1;
      presentInfo.pSwapchains        = &*swapchainBundle.swapchain;
      presentInfo.pImageIndices      = &imageIndex;

      try
      {
        auto presentRes = deviceBundle.graphicsQueue.presentKHR( presentInfo );
        if ( presentRes == vk::Result::eSuboptimalKHR || presentRes == vk::Result::eErrorOutOfDateKHR )
        {
          break;
        }
      }
      catch ( vk::OutOfDateKHRError const & )
      {
        break;
      }

      currentFrame = ( currentFrame + 1 ) % imageCount;
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
