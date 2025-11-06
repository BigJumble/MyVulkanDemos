
#include "GLFW/glfw3.h"
#include "data.hpp"
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

static void recreateSwapchain(
  vk::raii::PhysicalDevice & physicalDevice, core::SwapchainBundle & swapchainBundle, core::QueueFamilyIndices & queueFamilyIndices, VmaAllocator & allocator )
{
  int width = 0, height = 0;
  do
  {
    glfwGetFramebufferSize( global::obj::window.get(), &width, &height );
    glfwPollEvents();
  } while ( width == 0 || height == 0 );

  global::obj::device.waitIdle();

  core::SwapchainBundle old = std::move( swapchainBundle );
  swapchainBundle           = core::createSwapchain(
    physicalDevice,
    global::obj::device,
    global::obj::surface,
    vk::Extent2D{ static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) },
    queueFamilyIndices,
    &old.swapchain );
  global::state::screenSize = swapchainBundle.extent;
  // Recreate depth resources with new extent

  core::destroyTexture( global::obj::device, global::obj::allocator, global::obj::depthTexture );
  global::obj::depthTexture = core::createTexture(
    global::obj::device,
    global::obj::allocator,
    global::state::screenSize,
    vk::Format::eD32Sfloat,
    vk::ImageUsageFlagBits::eDepthStencilAttachment,
    vk::ImageAspectFlagBits::eDepth );

  core::destroyTexture( global::obj::device, global::obj::allocator, global::obj::basicTargetTexture );
  global::obj::basicTargetTexture = core::createTexture(
    global::obj::device,
    global::obj::allocator,
    global::state::screenSize,
    global::obj::swapchainBundle.imageFormat,
    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
    vk::ImageAspectFlagBits::eColor );
  // No need to recreate per-frame semaphores - they're independent of swapchain
}

int main()
{
  try
  {
    global::obj::instance = vk::raii::Instance( global::obj::context, core::createInfo );

    global::obj::physicalDevices = vk::raii::PhysicalDevices( global::obj::instance );

    global::obj::physicalDevice = core::selectPhysicalDevice( global::obj::physicalDevices );

    // global::obj::displayBundle = core::DisplayBundle( global::obj::instance);
    global::obj::window  = core::createWindow( global::obj::instance );
    global::obj::surface = core::createWindowSurface( global::obj::instance, global::obj::window.get() );

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

    // // Create depth resources
    // core::raii::DepthResources depthResources( global::obj::device, global::obj::allocator,  global::obj::swapchainBundle.extent );
    // // Create offscreen color target to render scene into
    // core::raii::ColorTarget offscreenColor( global::obj::device, global::obj::allocator,  global::obj::swapchainBundle.extent,
    // global::obj::swapchainBundle.imageFormat );

    core::raii::IMGUI imgui(
      global::obj::device,
      global::obj::instance,
      global::obj::physicalDevice,
      global::obj::queueFamilyIndices.graphicsFamily.value(),
      global::obj::graphicsQueue,
      global::obj::window.get(),
      static_cast<uint32_t>( global::obj::swapchainBundle.images.size() ),
      static_cast<uint32_t>( global::obj::swapchainBundle.images.size() ),
      global::obj::swapchainBundle.imageFormat,
      global::obj::depthTexture.format );

    //=========================================================
    // Vulkan setup
    //=========================================================

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

    VkBuffer          vertexBuffer;
    VmaAllocation     vertexBufferAllocation;
    VmaAllocationInfo vertexBufferAllocInfo;

    vmaCreateBuffer( global::obj::allocator, &bufferInfo, &allocInfo, &vertexBuffer, &vertexBufferAllocation, &vertexBufferAllocInfo );

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

    vmaCreateBuffer( global::obj::allocator, &instanceBufferInfo, &allocInfo, &instanceBuffer, &instanceBufferAllocation, &instanceBufferAllocInfo );

    // Copy instance data to buffer
    memcpy( instanceBufferAllocInfo.pMappedData, data::instancesPos.data(), static_cast<size_t>( instanceBufferSize ) );

    vk::CommandPoolCreateInfo cmdPoolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, global::obj::queueFamilyIndices.graphicsFamily.value() };
    vk::raii::CommandPool     commandPool{ global::obj::device, cmdPoolInfo };

    // Frames in flight - independent of swapchain image count
    constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    vk::CommandBufferAllocateInfo cmdInfo{ commandPool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>( MAX_FRAMES_IN_FLIGHT * 2 ) };
    vk::raii::CommandBuffers      cmds{ global::obj::device, cmdInfo };

    // Create per-frame binary semaphores for image acquisition and presentation
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence>     presentFences;

    imageAvailableSemaphores.reserve( MAX_FRAMES_IN_FLIGHT );
    renderFinishedSemaphores.reserve( MAX_FRAMES_IN_FLIGHT );
    presentFences.reserve( MAX_FRAMES_IN_FLIGHT );

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
      imageAvailableSemaphores.emplace_back( global::obj::device, vk::SemaphoreCreateInfo{} );
      renderFinishedSemaphores.emplace_back( global::obj::device, vk::SemaphoreCreateInfo{} );
      presentFences.emplace_back( global::obj::device, vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled } );
    }

    // Set up callbacks
    glfwSetKeyCallback( global::obj::window.get(), input::keyCallback );
    glfwSetMouseButtonCallback( global::obj::window.get(), input::mouseButtonCallback );
    input::previousCursorPosCallback = glfwSetCursorPosCallback( global::obj::window.get(), input::cursorPositionCallback );
    // glfwSetScrollCallback( displayBundle.window, input::scrollCallback );
    glfwSetFramebufferSizeCallback( global::obj::window.get(), input::framebufferResizeCallback );
    // glfwSetWindowSizeCallback( displayBundle.window, input::windowSizeCallback );
    // glfwSetCursorEnterCallback( displayBundle.window, input::cursorEnterCallback );

    // Optional: Set input modes
    glfwSetInputMode( global::obj::window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED );  // For FPS camera
    glfwSetInputMode( global::obj::window.get(), GLFW_STICKY_KEYS, GLFW_TRUE );

    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    size_t currentFrame = 0;

    while ( !glfwWindowShouldClose( global::obj::window.get() ) )
    {
      glfwPollEvents();

      if ( global::state::framebufferResized )
      {
        global::state::framebufferResized = false;
        recreateSwapchain(
          global::obj::physicalDevice,
          global::obj::swapchainBundle,
          global::obj::queueFamilyIndices,
          global::obj::allocator.allocator);
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
        // std::println( "imageIndex: {}", imageIndex );
        // Record command buffers for this frame: scene -> offscreen, then blit+imgui -> swapchain
        auto & cmdScene   = cmds[currentFrame * 2 + 0];
        auto & cmdOverlay = cmds[currentFrame * 2 + 1];
        pipelines::basic::recordCommandBufferOffscreen(
          cmdScene, shaderBundle, global::obj::basicTargetTexture, vertexBuffer, instanceBuffer, instanceCount, global::obj::depthTexture );
        pipelines::overlay::recordCommandBuffer( cmdOverlay, global::obj::basicTargetTexture, global::obj::swapchainBundle, imageIndex, true );

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

        currentFrame = ( currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
      }
      catch ( std::exception const & err )
      {
        isDebug( std::println( "Frame rendering exception (recreating swapchain): {}", err.what() ) );
        recreateSwapchain(
          global::obj::physicalDevice,
          global::obj::swapchainBundle,
          global::obj::queueFamilyIndices,
          global::obj::allocator.allocator);
        continue;
      }
    }

    global::obj::device.waitIdle();

    core::destroyTexture( global::obj::device, global::obj::allocator, global::obj::depthTexture );
    core::destroyTexture( global::obj::device, global::obj::allocator, global::obj::basicTargetTexture );
    // Cleanup VMA resources;
    vmaDestroyBuffer( global::obj::allocator, vertexBuffer, vertexBufferAllocation );
    vmaDestroyBuffer( global::obj::allocator, instanceBuffer, instanceBufferAllocation );
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
