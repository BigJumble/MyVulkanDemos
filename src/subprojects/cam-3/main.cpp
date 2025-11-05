
#include "GLFW/glfw3.h"
#include "state.hpp"
#include "data.hpp"

#include "objects.hpp"
#include "input.hpp"
#include "pipelines/basic.hpp"
#include "pipelines/overlay.hpp"
#include "state.hpp"
#include "ui.hpp"
#include "vulkan/vulkan.hpp"

#include <vulkan/vulkan_raii.hpp>

#define VMA_IMPLEMENTATION

#include <print>
#include <vk_mem_alloc.h>

static void recreateSwapchain(
  core::DisplayBundle &        displayBundle,
  vk::raii::PhysicalDevice &   physicalDevice,
  core::DeviceBundle &         deviceBundle,
  core::SwapchainBundle &      swapchainBundle,
  core::QueueFamilyIndices &   queueFamilyIndices,
  VmaAllocator &               allocator,
  core::raii::DepthResources & depthResources,
  core::raii::ColorTarget &    offscreenColor )
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
  global::state::screenSize = swapchainBundle.extent;
  // Recreate depth resources with new extent
  depthResources = core::raii::DepthResources( deviceBundle.device, allocator, swapchainBundle.extent );
  // Recreate offscreen color target matching swapchain
  offscreenColor = core::raii::ColorTarget( deviceBundle.device, allocator, swapchainBundle.extent, swapchainBundle.imageFormat );

  // No need to recreate per-frame semaphores - they're independent of swapchain
}

int main()
{
  try
  {
    global::obj::instance = vk::raii::Instance( global::obj::context, core::createInfo );

    global::obj::physicalDevices = vk::raii::PhysicalDevices( global::obj::instance );

    global::obj::physicalDevice = core::selectPhysicalDevice( global::obj::physicalDevices );

    global::obj::displayBundle = core::DisplayBundle( global::obj::instance);

    global::state::availablePresentModes = global::obj::physicalDevice.getSurfacePresentModesKHR( global::obj::displayBundle.surface );

    global::obj::queueFamilyIndices = core::findQueueFamilies( global::obj::physicalDevice, global::obj::displayBundle.surface );

    core::DeviceBundle deviceBundle = core::createDeviceWithQueues( global::obj::physicalDevice, global::obj::queueFamilyIndices, cfg::enabledFeaturesChain, cfg::getRequiredExtensions );

    core::SwapchainBundle swapchainBundle =
      core::createSwapchain( global::obj::physicalDevice, deviceBundle.device, global::obj::displayBundle.surface, global::state::screenSize, global::obj::queueFamilyIndices );
    global::state::screenSize = swapchainBundle.extent;
    core::raii::Allocator allocator( global::obj::instance, global::obj::physicalDevice, deviceBundle.device );

    // Create depth resources
    core::raii::DepthResources depthResources( deviceBundle.device, allocator, swapchainBundle.extent );
    // Create offscreen color target to render scene into
    core::raii::ColorTarget offscreenColor( deviceBundle.device, allocator, swapchainBundle.extent, swapchainBundle.imageFormat );

    core::raii::IMGUI imgui(
      deviceBundle.device,
      global::obj::instance,
      global::obj::physicalDevice,
      global::obj::queueFamilyIndices.graphicsFamily.value(),
      deviceBundle.graphicsQueue,
      global::obj::displayBundle.window,
      static_cast<uint32_t>( swapchainBundle.images.size() ),
      static_cast<uint32_t>( swapchainBundle.images.size() ),
      swapchainBundle.imageFormat,
      depthResources.depthFormat );

    //=========================================================
    // Vulkan setup
    //=========================================================

    core::raii::ShaderBundle shaderBundle(
      deviceBundle.device,
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

    VkBuffer          vertexBuffer;
    VmaAllocation     vertexBufferAllocation;
    VmaAllocationInfo vertexBufferAllocInfo;

    vmaCreateBuffer( allocator, &bufferInfo, &allocInfo, &vertexBuffer, &vertexBufferAllocation, &vertexBufferAllocInfo );

    // Copy vertex data to buffer (already mapped)
    memcpy( vertexBufferAllocInfo.pMappedData, data::triangleVertices.data(), static_cast<size_t>( bufferSize ) );

    uint32_t     instanceCount      = static_cast<uint32_t>( data::instancesPos.size() );
    VkDeviceSize instanceBufferSize = sizeof( data::InstanceData ) * data::instancesPos.size();

    VkBufferCreateInfo instanceBufferInfo = {};
    instanceBufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    instanceBufferInfo.size               = instanceBufferSize;
    instanceBufferInfo.usage              = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    instanceBufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer          instanceBuffer;
    VmaAllocation     instanceBufferAllocation;
    VmaAllocationInfo instanceBufferAllocInfo;

    vmaCreateBuffer( allocator, &instanceBufferInfo, &allocInfo, &instanceBuffer, &instanceBufferAllocation, &instanceBufferAllocInfo );

    // Copy instance data to buffer
    memcpy( instanceBufferAllocInfo.pMappedData, data::instancesPos.data(), static_cast<size_t>( instanceBufferSize ) );

    vk::CommandPoolCreateInfo cmdPoolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, global::obj::queueFamilyIndices.graphicsFamily.value() };
    vk::raii::CommandPool     commandPool{ deviceBundle.device, cmdPoolInfo };

    // Frames in flight - independent of swapchain image count
    constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    vk::CommandBufferAllocateInfo cmdInfo{ commandPool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>( MAX_FRAMES_IN_FLIGHT * 2 ) };
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

    // Set up callbacks
    glfwSetKeyCallback( global::obj::displayBundle.window, input::keyCallback );
    glfwSetMouseButtonCallback( global::obj::displayBundle.window, input::mouseButtonCallback );
    input::previousCursorPosCallback = glfwSetCursorPosCallback( global::obj::displayBundle.window, input::cursorPositionCallback );
    // glfwSetScrollCallback( displayBundle.window, input::scrollCallback );
    glfwSetFramebufferSizeCallback( global::obj::displayBundle.window, input::framebufferResizeCallback );
    // glfwSetWindowSizeCallback( displayBundle.window, input::windowSizeCallback );
    // glfwSetCursorEnterCallback( displayBundle.window, input::cursorEnterCallback );

    // Optional: Set input modes
    glfwSetInputMode( global::obj::displayBundle.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );  // For FPS camera
    glfwSetInputMode( global::obj::displayBundle.window, GLFW_STICKY_KEYS, GLFW_TRUE );

    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    size_t currentFrame = 0;

    while ( !glfwWindowShouldClose( global::obj::displayBundle.window ) )
    {
      glfwPollEvents();

      if ( global::state::framebufferResized )
      {
        global::state::framebufferResized = false;
        recreateSwapchain(
          global::obj::displayBundle, global::obj::physicalDevice, deviceBundle, swapchainBundle, global::obj::queueFamilyIndices, allocator.allocator, depthResources, offscreenColor );
        continue;
      }

      if ( !global::state::fpvMode )
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
          throw std::runtime_error( "acquire.result: " + std::to_string( static_cast<int>( acquire.result ) ) );
        }
        uint32_t imageIndex = acquire.value;

        // Only reset the fence after successful image acquisition to prevent deadlock on exception
        deviceBundle.device.resetFences( { *presentFence } );
        // std::println( "imageIndex: {}", imageIndex );
        // Record command buffers for this frame: scene -> offscreen, then blit+imgui -> swapchain
        auto & cmdScene   = cmds[currentFrame * 2 + 0];
        auto & cmdOverlay = cmds[currentFrame * 2 + 1];
        pipelines::basic::recordCommandBufferOffscreen( cmdScene, shaderBundle, offscreenColor, vertexBuffer, instanceBuffer, instanceCount, depthResources );
        pipelines::overlay::recordCommandBuffer( cmdOverlay, offscreenColor, swapchainBundle, imageIndex, true );

        // Submit command buffer waiting on imageAvailable, signal renderFinished and timeline
        // uint64_t renderCompleteValue = ++currentTimelineValue;

        std::array<vk::SemaphoreSubmitInfo, 1> waitSemaphoreInfos = {
          vk::SemaphoreSubmitInfo{}.setSemaphore( *imageAvailable ).setStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        };

        std::array<vk::SemaphoreSubmitInfo, 1> signalSemaphoreInfos = {
          vk::SemaphoreSubmitInfo{}.setSemaphore( *renderFinished ).setStageMask( vk::PipelineStageFlagBits2::eAllCommands ),
          // vk::SemaphoreSubmitInfo{}.setSemaphore( *syncSemaphore ).setValue( renderCompleteValue ).setStageMask( vk::PipelineStageFlagBits2::eAllCommands )
        };

        std::array<vk::CommandBufferSubmitInfo, 2> cmdBufferInfos{ vk::CommandBufferSubmitInfo{}.setCommandBuffer( *cmdScene ),
                                                                   vk::CommandBufferSubmitInfo{}.setCommandBuffer( *cmdOverlay ) };

        vk::SubmitInfo2 submitInfo{};
        submitInfo.setCommandBufferInfoCount( cmdBufferInfos.size() )
          .setPCommandBufferInfos( cmdBufferInfos.data() )
          .setWaitSemaphoreInfos( waitSemaphoreInfos )
          .setSignalSemaphoreInfos( signalSemaphoreInfos );

          deviceBundle.graphicsQueue.submit2( submitInfo );

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
          .setPSwapchains( &*swapchainBundle.swapchain )
          .setPImageIndices( &imageIndex );

        auto presentRes = deviceBundle.graphicsQueue.presentKHR( presentInfo );

        if ( presentRes == vk::Result::eSuboptimalKHR || presentRes == vk::Result::eErrorOutOfDateKHR )
        {
          throw std::runtime_error( "presentRes: " + std::to_string( static_cast<int>( presentRes ) ) );
        }

        currentFrame = ( currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
      }
      catch ( std::exception const & err )
      {
        isDebug( std::println( "Frame rendering exception (recreating swapchain): {}", err.what() ) );
        recreateSwapchain(
          global::obj::displayBundle, global::obj::physicalDevice, deviceBundle, swapchainBundle, global::obj::queueFamilyIndices, allocator.allocator, depthResources, offscreenColor );
        continue;
      }
    }

    deviceBundle.device.waitIdle();

    // Cleanup VMA resources;
    vmaDestroyBuffer( allocator, vertexBuffer, vertexBufferAllocation );
    vmaDestroyBuffer( allocator, instanceBuffer, instanceBufferAllocation );
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
