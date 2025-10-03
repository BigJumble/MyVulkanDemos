#include "helper.hpp"
#include "settings.hpp"
#include <print>
#include <array>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

static std::string AppName    = "01_InitInstance";
static std::string EngineName = "Vulkan.hpp";

int main()
{
  /* VULKAN_HPP_KEY_START */

  try
  {
    // the very beginning: instantiate a context
    vk::raii::Context context;

    // create an Instance
    vk::raii::Instance instance( context, core::createInstanceCreateInfo( AppName, EngineName, {}, core::InstanceExtensions ) );

    isDebug( vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger( instance, core::createDebugUtilsMessengerCreateInfo() ); );

    vk::raii::PhysicalDevices physicalDevices( instance );

    vk::raii::PhysicalDevice physicalDevice = core::selectPhysicalDevice( physicalDevices );

    core::DisplayBundle Display( instance, "MyEngine", vk::Extent2D( 1280, 720 ) );

    core::QueueFamilyIndices indices = core::findQueueFamilies( physicalDevice, Display.surface );

    core::DeviceBundle deviceBundle = core::createDeviceWithQueues( physicalDevice, indices );

    core::SwapchainBundle swapchain = core::createSwapchain( physicalDevice, deviceBundle.device, Display.surface, Display.extent, indices );

    isDebug(
      std::println(
        "Swapchain created: {} images, format {}, extent {}x{}",
        swapchain.images.size(),
        vk::to_string( swapchain.imageFormat ),
        swapchain.extent.width,
        swapchain.extent.height ); );

    // Load vertex and fragment shaders
    std::vector<uint32_t> vertShaderCode = core::readSpirvFile( "shaders/triangle.vert.spv" );
    std::vector<uint32_t> fragShaderCode = core::readSpirvFile( "shaders/triangle.frag.spv" );

    vk::raii::ShaderModule vertShaderModule = core::createShaderModule( deviceBundle.device, vertShaderCode );
    vk::raii::ShaderModule fragShaderModule = core::createShaderModule( deviceBundle.device, fragShaderCode );

    // Create a render pass matching the swapchain image format
    vk::raii::RenderPass renderPass = core::createRenderPass( deviceBundle.device, swapchain.imageFormat );

    // Create a pipeline layout (no descriptors for this simple triangle)
    vk::raii::PipelineLayout pipelineLayout = core::createPipelineLayout( deviceBundle.device );

    // Create the graphics pipeline using the loaded shader modules
    vk::raii::Pipeline graphicsPipeline = core::createGraphicsPipeline( deviceBundle.device, renderPass, pipelineLayout, swapchain.extent, vertShaderModule, fragShaderModule );

    // Create framebuffers for each swapchain image view
    std::vector<vk::raii::Framebuffer> framebuffers = core::createFramebuffers( deviceBundle.device, renderPass, swapchain.extent, swapchain.imageViews );

    // Create command pool and buffers (one per framebuffer)
    core::CommandResources commandResources = core::createCommandResources( deviceBundle.device, deviceBundle.indices.graphicsFamily, framebuffers.size() );

    // Record commands to draw the triangle into each framebuffer (will be re-recorded each frame when rendering ImGui)
    core::recordTriangleCommands( commandResources.buffers, renderPass, framebuffers.data(), framebuffers.size(), swapchain.extent, graphicsPipeline );

    // ImGui: create descriptor pool
    std::array<vk::DescriptorPoolSize, 11> imguiPoolSizes = {
      vk::DescriptorPoolSize{ vk::DescriptorType::eSampler, 1000 },
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
    vk::DescriptorPoolCreateInfo imguiPoolInfo{};
    imguiPoolInfo.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    imguiPoolInfo.maxSets       = 1000 * static_cast<uint32_t>( imguiPoolSizes.size() );
    imguiPoolInfo.poolSizeCount = static_cast<uint32_t>( imguiPoolSizes.size() );
    imguiPoolInfo.pPoolSizes    = imguiPoolSizes.data();
    vk::raii::DescriptorPool imguiDescriptorPool{ deviceBundle.device, imguiPoolInfo };

    // ImGui: initialize context and backends
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan( Display.window, true );

    ImGui_ImplVulkan_InitInfo imguiInitInfo{};
    imguiInitInfo.Instance       = *instance;
    imguiInitInfo.PhysicalDevice = *physicalDevice;
    imguiInitInfo.Device         = *deviceBundle.device;
    imguiInitInfo.QueueFamily    = deviceBundle.indices.graphicsFamily;
    imguiInitInfo.Queue          = *deviceBundle.graphicsQueue;
    imguiInitInfo.DescriptorPool = *imguiDescriptorPool;
    imguiInitInfo.RenderPass     = *renderPass;
    imguiInitInfo.MinImageCount  = static_cast<uint32_t>( framebuffers.size() );
    imguiInitInfo.ImageCount     = static_cast<uint32_t>( framebuffers.size() );
    imguiInitInfo.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;
    imguiInitInfo.UseDynamicRendering = false;
    ImGui_ImplVulkan_Init( &imguiInitInfo );

    // ImGui: recent backends build fonts automatically during NewFrame; no manual upload required

    // Create synchronization objects for 2 frames in flight
    core::SyncObjects syncObjects = core::createSyncObjects( deviceBundle.device, 2 );

    // Main render loop
    size_t currentFrame = 0;
    bool resize = false;
    while ( !glfwWindowShouldClose( Display.window ) )
    {
      glfwPollEvents();

      // Check if window was resized/minimized
      int width, height;
      glfwGetFramebufferSize( Display.window, &width, &height );
      if ( width == 0 || height == 0 )
      {
        glfwWaitEvents();
        continue;
      }

      if ( width != Display.extent.width || height != Display.extent.height )
      {
        // Update extent and recreate swapchain
        Display.extent = vk::Extent2D( static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) );
        deviceBundle.device.waitIdle();

        swapchain = core::createSwapchain( physicalDevice, deviceBundle.device, Display.surface, Display.extent, indices, &swapchain.swapchain );

        // Recreate framebuffers with new swapchain
        framebuffers = core::createFramebuffers( deviceBundle.device, renderPass, Display.extent, swapchain.imageViews );

        // Recreate command resources with new framebuffer count
        commandResources = core::createCommandResources( deviceBundle.device, deviceBundle.indices.graphicsFamily, framebuffers.size() );

        // Re-record commands for new framebuffers (pipeline uses dynamic viewport/scissor)
        core::recordTriangleCommands( commandResources.buffers, renderPass, framebuffers.data(), framebuffers.size(), Display.extent, graphicsPipeline );

        // Reset current frame to avoid issues with sync objects
        currentFrame = 0;
        resize = false;
      }

      // Start ImGui frame and show FPS window
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      if ( ImGui::Begin( "Stats" ) )
      {
        ImGui::Text( "FPS: %.1f", ImGui::GetIO().Framerate );
        ImGui::Button("adad");
      }
      ImGui::End();
      ImGui::Render();

      // Draw one frame with ImGui
      try
      {
        (void)deviceBundle.device.waitForFences( *syncObjects.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX );

        uint32_t imageIndex = 0;
        auto     acq        = swapchain.swapchain.acquireNextImage( UINT64_MAX, *syncObjects.imageAvailable[currentFrame], nullptr );
        if ( acq.first != vk::Result::eSuccess && acq.first != vk::Result::eSuboptimalKHR )
        {
          throw std::runtime_error( "Failed to acquire swapchain image" );
        }
        imageIndex = acq.second;

        deviceBundle.device.resetFences( *syncObjects.inFlightFences[currentFrame] );

        auto & cb = commandResources.buffers[imageIndex];
        cb.reset();
        cb.begin( vk::CommandBufferBeginInfo{} );

        vk::ClearValue          clearColor = vk::ClearColorValue{ std::array<float, 4>{ 0.02f, 0.02f, 0.03f, 1.0f } };
        vk::RenderPassBeginInfo rpBegin{};
        rpBegin.renderPass        = *renderPass;
        rpBegin.framebuffer       = *framebuffers[imageIndex];
        rpBegin.renderArea.offset = vk::Offset2D{ 0, 0 };
        rpBegin.renderArea.extent = swapchain.extent;
        rpBegin.clearValueCount   = 1;
        rpBegin.pClearValues      = &clearColor;

        cb.beginRenderPass( rpBegin, vk::SubpassContents::eInline );
        cb.bindPipeline( vk::PipelineBindPoint::eGraphics, *graphicsPipeline );
        cb.draw( 3, 1, 0, 0 );
        ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), *cb );
        cb.endRenderPass();
        cb.end();

        vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo         submitInfo{};
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &*syncObjects.imageAvailable[currentFrame];
        submitInfo.pWaitDstStageMask    = &waitStages;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &*cb;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &*syncObjects.renderFinished[currentFrame];

        deviceBundle.graphicsQueue.submit( submitInfo, *syncObjects.inFlightFences[currentFrame] );

        vk::PresentInfoKHR presentInfo{};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &*syncObjects.renderFinished[currentFrame];
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &*swapchain.swapchain;
        presentInfo.pImageIndices      = &imageIndex;
        (void)deviceBundle.presentQueue.presentKHR( presentInfo );

        currentFrame = ( currentFrame + 1 ) % syncObjects.imageAvailable.size();
      }
      catch ( std::exception const & err )
      {
        isDebug( std::println( "drawFrame exception: {}", err.what() ) );
        resize = true;
      }
    }

    // Cleanup ImGui
    deviceBundle.device.waitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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
