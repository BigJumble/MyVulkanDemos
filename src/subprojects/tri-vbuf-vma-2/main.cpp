#include "bootstrap.hpp"
#include "settings.hpp"

#define VMA_IMPLEMENTATION
#include "offload/allocator.hpp"
#include "offload/buffer.hpp"
#include "offload/rendering.hpp"
#include "offload/shader.hpp"
#include "offload/swapchain.hpp"
#include "offload/sync.hpp"
#include "offload/types.hpp"

#include <print>
#include <vk_mem_alloc.h>

constexpr std::string_view AppName              = "MyApp";
constexpr std::string_view EngineName           = "MyEngine";
constexpr size_t           MAX_FRAMES_IN_FLIGHT = 2;

int main()
{
  /* VULKAN_HPP_KEY_START */
  isDebug( std::println( "LOADING UP CAMERA INSTANCING EXAMPLE!\n" ); );
  try
  {
    // Create Vulkan context and instance
    vk::raii::Context  context;
    vk::raii::Instance instance = core::createInstance( context, std::string( AppName ), std::string( EngineName ) );

    // Select physical device
    vk::raii::PhysicalDevices physicalDevices( instance );
    vk::raii::PhysicalDevice  physicalDevice = core::selectPhysicalDevice( physicalDevices );

    // Create window and surface
    core::DisplayBundle displayBundle( instance, "MyEngine", vk::Extent2D( 1280, 720 ) );

    // Find queue families and create logical device
    core::QueueFamilyIndices queueFamilyIndices = core::findQueueFamilies( physicalDevice, displayBundle.surface );
    core::DeviceBundle       deviceBundle       = core::createDeviceWithQueues( physicalDevice, queueFamilyIndices );

    // Create swapchain
    core::SwapchainBundle swapchainBundle = core::createSwapchain( physicalDevice, deviceBundle.device, displayBundle.surface, displayBundle.extent, queueFamilyIndices );

    // Create VMA allocator
    VmaAllocator allocator = offload::createAllocator( instance, physicalDevice, deviceBundle.device );

    // Create pipeline layout and shaders
    vk::raii::PipelineLayout pipelineLayout = offload::createPipelineLayout( deviceBundle.device );

    vk::PushConstantRange pushConstantRange{};
    pushConstantRange.setStageFlags( vk::ShaderStageFlagBits::eVertex ).setSize( sizeof( offload::PushConstants ) ).setOffset( 0 );

    offload::ShaderObjects shaders = offload::createShaderObjects( deviceBundle.device, pushConstantRange );

    // Create vertex buffer
    std::array<offload::Vertex, 3> vertices = { offload::Vertex{ glm::vec2( 0.0f, -0.5f ), glm::vec3( 1.0f, 0.5f, 0.5f ) },
                                                offload::Vertex{ glm::vec2( 0.5f, 0.5f ), glm::vec3( 0.5f, 1.0f, 0.5f ) },
                                                offload::Vertex{ glm::vec2( -0.5f, 0.5f ), glm::vec3( 0.5f, 0.5f, 1.0f ) } };

    offload::BufferAllocation vertexBuffer = offload::createVertexBuffer( allocator, vertices );

    // Create command pool and buffers
    vk::CommandPoolCreateInfo cmdPoolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndices.graphicsFamily.value() };
    vk::raii::CommandPool     commandPool{ deviceBundle.device, cmdPoolInfo };

    vk::CommandBufferAllocateInfo cmdInfo{ commandPool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT };
    vk::raii::CommandBuffers      cmds{ deviceBundle.device, cmdInfo };

    // Create synchronization objects
    offload::FrameSyncObjects syncObjects = offload::createFrameSyncObjects( deviceBundle.device, MAX_FRAMES_IN_FLIGHT );

    // Setup window resize callback
    bool framebufferResized = false;
    glfwSetWindowUserPointer( displayBundle.window, &framebufferResized );
    glfwSetFramebufferSizeCallback( displayBundle.window, offload::framebufferResizeCallback );

    // Main render loop
    size_t currentFrame = 0;
    while ( !glfwWindowShouldClose( displayBundle.window ) )
    {
      glfwPollEvents();

      // Handle window resize
      if ( framebufferResized )
      {
        framebufferResized = false;
        offload::recreateSwapchain( displayBundle, physicalDevice, deviceBundle, swapchainBundle, queueFamilyIndices );
        continue;
      }

      try
      {
        // Get synchronization objects for current frame
        auto & imageAvailable = syncObjects.imageAvailableSemaphores[currentFrame];
        auto & renderFinished = syncObjects.renderFinishedSemaphores[currentFrame];
        auto & presentFence   = syncObjects.presentFences[currentFrame];

        // Wait for previous frame to finish presenting
        (void)deviceBundle.device.waitForFences( { *presentFence }, VK_TRUE, UINT64_MAX );

        // Acquire next swapchain image
        auto acquire = swapchainBundle.swapchain.acquireNextImage( UINT64_MAX, *imageAvailable, nullptr );
        if ( acquire.result == vk::Result::eErrorOutOfDateKHR )
        {
          throw acquire.result;
        }
        uint32_t imageIndex = acquire.value;

        // Reset fence after successful acquisition
        deviceBundle.device.resetFences( { *presentFence } );

        // Record and submit command buffer
        auto & cmd = cmds[currentFrame];
        offload::recordCommandBuffer( cmd, shaders.vertShader, shaders.fragShader, swapchainBundle, imageIndex, pipelineLayout, vertexBuffer.buffer );

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

        // Present rendered image
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
        offload::recreateSwapchain( displayBundle, physicalDevice, deviceBundle, swapchainBundle, queueFamilyIndices );
        continue;
      }
    }

    // Cleanup
    deviceBundle.device.waitIdle();
    vmaDestroyBuffer( allocator, vertexBuffer.buffer, vertexBuffer.allocation );
    vmaDestroyAllocator( allocator );
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
