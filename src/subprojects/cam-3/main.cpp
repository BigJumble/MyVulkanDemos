
#include "GLFW/glfw3.h"
#include "data.hpp"
#include "glm/geometric.hpp"
#include "input.hpp"
#include "objects.hpp"
#include "pipelines/basic.hpp"
#include "pipelines/overlay.hpp"
#include "setup.hpp"
#include "state.hpp"
#include "structs.hpp"
#include "ui.hpp"
#include "vulkan/vulkan.hpp"

#include <vulkan/vulkan_raii.hpp>

#define VMA_IMPLEMENTATION

#include <print>
#include <vk_mem_alloc.h>

int main()
{
  try
  {
    //=========================================================
    // Vulkan setup
    //=========================================================

    global::obj::instance = vk::raii::Instance( global::obj::context, core::createInfo );

    global::obj::physicalDevices = vk::raii::PhysicalDevices( global::obj::instance );

    global::obj::physicalDevice = core::selectPhysicalDevice( global::obj::physicalDevices );

    global::obj::window  = core::raii::Window( global::obj::instance );
    global::obj::surface = core::createWindowSurface( global::obj::instance, global::obj::window );

    global::state::availablePresentModes = global::obj::physicalDevice.getSurfacePresentModesKHR( global::obj::surface );

    global::obj::queueFamilyIndices = core::findQueueFamilies( global::obj::physicalDevice, global::obj::surface );

    global::obj::device =
      core::createDevice( global::obj::physicalDevice, global::obj::queueFamilyIndices, cfg::enabledFeaturesChain, cfg::getRequiredExtensions );

    global::obj::graphicsQueue = vk::raii::Queue( global::obj::device, global::obj::queueFamilyIndices.graphicsFamily.value(), 0 );
    global::obj::presentQueue  = vk::raii::Queue( global::obj::device, global::obj::queueFamilyIndices.presentFamily.value(), 0 );
    global::obj::computeQueue  = vk::raii::Queue( global::obj::device, global::obj::queueFamilyIndices.computeFamily.value(), 0 );

    global::obj::swapchainBundle = core::createSwapchain(
      global::obj::physicalDevice, global::obj::device, global::obj::surface, global::state::screenSize, global::obj::queueFamilyIndices );

    global::state::screenSize = global::obj::swapchainBundle.extent;

    global::obj::allocator = core::raii::Allocator( global::obj::instance, global::obj::physicalDevice, global::obj::device );

    global::obj::descriptorPool = core::createDescriptorPool( global::obj::device );

    //=========================================================
    // ImGui setup
    //=========================================================

    core::initImGui(
      global::obj::device,
      global::obj::instance,
      global::obj::physicalDevice,
      global::obj::queueFamilyIndices.graphicsFamily.value(),
      global::obj::graphicsQueue,
      global::obj::window,
      static_cast<uint32_t>( global::obj::swapchainBundle.images.size() ),
      static_cast<uint32_t>( global::obj::swapchainBundle.images.size() ),
      global::obj::swapchainBundle.imageFormat,
      global::obj::descriptorPool );

    // Set up callbacks
    input::previousKeyCallback         = glfwSetKeyCallback( global::obj::window, input::keyCallback );
    input::previousMouseButtonCallback = glfwSetMouseButtonCallback( global::obj::window, input::mouseButtonCallback );
    input::previousCursorPosCallback   = glfwSetCursorPosCallback( global::obj::window, input::cursorPositionCallback );
    input::previousScrollCallback      = glfwSetScrollCallback( global::obj::window, input::scrollCallback );

    // input::previousWindowFocusCallback = glfwSetWindowFocusCallback( global::obj::window, []( GLFWwindow *, int ) {} );
    input::previousCursorEnterCallback = glfwSetCursorEnterCallback( global::obj::window, input::cursorEnterCallback );
    // input::previousCharCallback        = glfwSetCharCallback( global::obj::window, []( GLFWwindow *, unsigned int ) {} );
    // input::previousMonitorCallback     = glfwSetMonitorCallback( []( GLFWmonitor *, int ) {} );

    glfwSetFramebufferSizeCallback( global::obj::window, input::framebufferResizeCallback );

    glfwSetInputMode( global::obj::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
    glfwSetInputMode( global::obj::window, GLFW_STICKY_KEYS, GLFW_TRUE );

    //=========================================================
    // Scene setup
    //=========================================================

    global::obj::depthTexture = core::createTexture(
      global::obj::device,
      global::obj::allocator,
      global::state::screenSize,
      vk::Format::eD32Sfloat,
      vk::ImageUsageFlagBits::eDepthStencilAttachment,
      vk::ImageAspectFlagBits::eDepth );

    global::obj::basicTargetTexture = core::createTexture(
      global::obj::device,
      global::obj::allocator,
      global::state::screenSize,
      global::obj::swapchainBundle.imageFormat,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageAspectFlagBits::eColor );

    core::raii::ShaderBundle shaderBundle(
      global::obj::device,
      { "triangle.vert" },
      { "triangle.frag" },
      vk::PushConstantRange{ vk::ShaderStageFlagBits::eVertex, 0, sizeof( data::PushConstants ) } );

    VkDeviceSize bufferSize = sizeof( data::triangleVertices );

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size               = bufferSize;
    bufferInfo.usage              = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    vmaCreateBuffer(
      global::obj::allocator,
      &bufferInfo,
      &allocInfo,
      &global::obj::vertexBuffer.buffer,
      &global::obj::vertexBuffer.allocation,
      &global::obj::vertexBuffer.allocationInfo );
    global::obj::vertexBuffer.size = bufferSize;

    // Copy vertex data to buffer (already mapped)
    memcpy( global::obj::vertexBuffer.allocationInfo.pMappedData, data::triangleVertices.data(), static_cast<size_t>( bufferSize ) );

    uint32_t     instanceCount      = static_cast<uint32_t>( data::instancesPos.size() );
    VkDeviceSize instanceBufferSize = sizeof( data::InstanceData ) * data::instancesPos.size();

    VkBufferCreateInfo instanceBufferInfo = {};
    instanceBufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    instanceBufferInfo.size               = instanceBufferSize;
    instanceBufferInfo.usage              = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    instanceBufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    vmaCreateBuffer(
      global::obj::allocator,
      &instanceBufferInfo,
      &allocInfo,
      &global::obj::instanceBuffer.buffer,
      &global::obj::instanceBuffer.allocation,
      &global::obj::instanceBuffer.allocationInfo );
    global::obj::instanceBuffer.size = instanceBufferSize;

    // Copy instance data to buffer
    memcpy( global::obj::instanceBuffer.allocationInfo.pMappedData, data::instancesPos.data(), static_cast<size_t>( instanceBufferSize ) );

    vk::CommandPoolCreateInfo cmdPoolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, global::obj::queueFamilyIndices.graphicsFamily.value() };
    global::obj::commandPool = vk::raii::CommandPool{ global::obj::device, cmdPoolInfo };

    {
      // Allocate command buffers as individual singletons per frame slot
      vk::CommandBufferAllocateInfo allocInfoCmd{ global::obj::commandPool,
                                                  vk::CommandBufferLevel::ePrimary,
                                                  static_cast<uint32_t>( global::state::MAX_FRAMES_IN_FLIGHT ) };
      global::obj::cmdScene   = vk::raii::CommandBuffers( global::obj::device, allocInfoCmd );
      global::obj::cmdOverlay = vk::raii::CommandBuffers( global::obj::device, allocInfoCmd );

      // Create per-frame synchronization objects
      global::obj::frames.reserve( global::state::MAX_FRAMES_IN_FLIGHT );
      for ( size_t i = 0; i < global::state::MAX_FRAMES_IN_FLIGHT; ++i )
      {
        global::obj::frames.emplace_back(
          core::FrameInFlight{ { global::obj::device, vk::SemaphoreCreateInfo{} },
                               { global::obj::device, vk::SemaphoreCreateInfo{} },
                               { global::obj::device, vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled } } } );
      }
    }

    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    size_t currentFrame = 0;

    while ( !glfwWindowShouldClose( global::obj::window ) )
    {
      glfwPollEvents();

      // Compute the camera direction in the XZ plane from cameraRotation.y (yaw)
      float     yaw     = global::state::cameraRotation.x;
      glm::vec3 forward = glm::vec3( std::sin( yaw ), 0.0f, std::cos( yaw ) );
      glm::vec3   left      = glm::cross( glm::vec3( 0.0f, 1.0f, 0.0f ), forward );
      const float moveSpeed = 0.1f;
      glm::vec3   moveStep  = glm::vec3( 0 );

      if ( global::state::keysPressed.contains( core::Key::A ) )
      {
        moveStep += left;
      }
      if ( global::state::keysPressed.contains( core::Key::D ) )
      {
        moveStep -= left;
      }
      if ( global::state::keysPressed.contains( core::Key::W ) )
      {
        moveStep += forward;
      }
      if ( global::state::keysPressed.contains( core::Key::S ) )
      {
        moveStep -= forward;
      }
      if ( global::state::keysPressed.contains( core::Key::Space ) )
      {
        moveStep.y += 1;
      }
      if ( global::state::keysPressed.contains( core::Key::LeftShift ) )
      {
        moveStep.y -= 1;
      }
      // std::println("{} {}", moveStep.x, glm::normalize(moveStep).x);
      if ( moveStep != glm::vec3( 0.0f ) )
        global::state::cameraPosition += glm::normalize( moveStep ) * moveSpeed;

      if ( global::state::framebufferResized )
      {
        global::state::framebufferResized = false;
        core::recreateSwapchain(
          global::obj::device,
          global::obj::physicalDevice,
          global::obj::surface,
          global::obj::queueFamilyIndices,
          global::obj::swapchainBundle,
          global::state::screenSize,
          global::obj::allocator.allocator,
          global::obj::depthTexture,
          global::obj::basicTargetTexture,
          global::obj::window );
        continue;
      }

      if ( global::state::imguiMode )
      {
        // Start ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ui::renderStatsWindow();
        ui::renderPresentModeWindow();
        ui::renderPipelineStateWindow();
        ui::logging();

        ImGui::Render();
      }

      try
      {
        // Get synchronization objects for current frame in flight
        auto & imageAvailable = global::obj::frames[currentFrame].imageAvailable;
        auto & renderFinished = global::obj::frames[currentFrame].renderFinished;
        auto & presentFence   = global::obj::frames[currentFrame].presentFence;

        // Wait for the presentation fence from previous use of this frame slot
        // Wait for the present fence to be signaled before reusing this frame slot
        (void)global::obj::device.waitForFences( { *presentFence }, VK_TRUE, UINT64_MAX );

        // Acquire next swapchain image using the imageAvailable semaphore
        auto acquire = global::obj::swapchainBundle.swapchain.acquireNextImage( UINT64_MAX, *imageAvailable, nullptr );
        if ( acquire.result == vk::Result::eErrorOutOfDateKHR )
        {
          throw std::runtime_error( "acquire.result: " + std::to_string( static_cast<int>( acquire.result ) ) );
        }
        uint32_t imageIndex = acquire.value;

        // Only reset the fence after successful image acquisition to prevent deadlock on exception
        global::obj::device.resetFences( { *presentFence } );

        // Record command buffers for this frame: scene -> offscreen, then blit+imgui -> swapchain
        auto & cmdScene   = global::obj::cmdScene[currentFrame];
        auto & cmdOverlay = global::obj::cmdOverlay[currentFrame];

        pipelines::basic::recordCommandBufferOffscreen(
          cmdScene,
          shaderBundle,
          global::obj::basicTargetTexture,
          global::obj::vertexBuffer.buffer,
          global::obj::instanceBuffer.buffer,
          instanceCount,
          global::obj::depthTexture );

        pipelines::overlay::recordCommandBuffer( cmdOverlay, global::obj::basicTargetTexture, global::obj::swapchainBundle, imageIndex );

        // Submit command buffer waiting on imageAvailable, signal renderFinished and timeline
        // uint64_t renderCompleteValue = ++currentTimelineValue;

        std::array<vk::SemaphoreSubmitInfo, 1> waitSemaphoreInfos = {
          vk::SemaphoreSubmitInfo{}.setSemaphore( *imageAvailable ).setStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        };

        std::array<vk::SemaphoreSubmitInfo, 1> signalSemaphoreInfos = {
          vk::SemaphoreSubmitInfo{}.setSemaphore( *renderFinished ).setStageMask( vk::PipelineStageFlagBits2::eBottomOfPipe ),
          // vk::SemaphoreSubmitInfo{}.setSemaphore( *syncSemaphore ).setValue( renderCompleteValue ).setStageMask( vk::PipelineStageFlagBits2::eAllCommands )
        };

        std::array<vk::CommandBufferSubmitInfo, 2> cmdBufferInfos{ vk::CommandBufferSubmitInfo{}.setCommandBuffer( *cmdScene ),
                                                                   vk::CommandBufferSubmitInfo{}.setCommandBuffer( *cmdOverlay ) };

        vk::SubmitInfo2 submitInfo{};
        submitInfo.setCommandBufferInfoCount( cmdBufferInfos.size() )
          .setPCommandBufferInfos( cmdBufferInfos.data() )
          .setWaitSemaphoreInfos( waitSemaphoreInfos )
          .setSignalSemaphoreInfos( signalSemaphoreInfos );

        global::obj::graphicsQueue.submit2( submitInfo );

        vk::SwapchainPresentModeInfoEXT presentModeInfo{};
        presentModeInfo.setSwapchainCount( 1 );
        presentModeInfo.setPPresentModes( &global::state::presentMode );

        vk::SwapchainPresentFenceInfoEXT presentFenceInfo{};
        presentFenceInfo.setSwapchainCount( 1 ).setPFences( &*presentFence );
        presentFenceInfo.setPNext( &presentModeInfo );

        vk::PresentInfoKHR presentInfo{};
        presentInfo.setPNext( &presentFenceInfo )
          .setWaitSemaphoreCount( 1 )
          .setPWaitSemaphores( &*renderFinished )
          .setSwapchainCount( 1 )
          .setPSwapchains( &*global::obj::swapchainBundle.swapchain )
          .setPImageIndices( &imageIndex );

        auto presentRes = global::obj::graphicsQueue.presentKHR( presentInfo );

        if ( presentRes == vk::Result::eSuboptimalKHR || presentRes == vk::Result::eErrorOutOfDateKHR )
        {
          throw std::runtime_error( "presentRes: " + std::to_string( static_cast<int>( presentRes ) ) );
        }

        currentFrame = ( currentFrame + 1 ) % global::state::MAX_FRAMES_IN_FLIGHT;

        global::state::keysDown.clear();
        global::state::keysUp.clear();
      }
      catch ( std::exception const & err )
      {
        isDebug( std::println( "Frame rendering exception (recreating swapchain): {}", err.what() ) );
        core::recreateSwapchain(
          global::obj::device,
          global::obj::physicalDevice,
          global::obj::surface,
          global::obj::queueFamilyIndices,
          global::obj::swapchainBundle,
          global::state::screenSize,
          global::obj::allocator.allocator,
          global::obj::depthTexture,
          global::obj::basicTargetTexture,
          global::obj::window );
        continue;
      }
    }

    global::obj::device.waitIdle();

    core::shutdownImGui();

    core::destroyTexture( global::obj::device, global::obj::allocator, global::obj::depthTexture );
    core::destroyTexture( global::obj::device, global::obj::allocator, global::obj::basicTargetTexture );
    // Cleanup VMA resources;
    vmaDestroyBuffer( global::obj::allocator, global::obj::vertexBuffer.buffer, global::obj::vertexBuffer.allocation );
    vmaDestroyBuffer( global::obj::allocator, global::obj::instanceBuffer.buffer, global::obj::instanceBuffer.allocation );
    // vmaDestroyAllocator( allocator );
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
