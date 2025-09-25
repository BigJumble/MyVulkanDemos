#include "helper.hpp"
#include "settings.hpp"
#include <limits>
#include <print>
// #include "settings.hpp"

// #include <vulkan/vulkan_core.h>
// #include <vulkan/vulkan_raii.hpp>
// #include <vulkan/vulkan_structs.hpp>
// #include <print>
// #include <vulkan/vulkan_raii.hpp>

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

    // Record commands to draw the triangle into each framebuffer
    core::recordTriangleCommands( commandResources.buffers, renderPass, framebuffers.data(), framebuffers.size(), swapchain.extent, graphicsPipeline );

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

        // Re-record commands for new framebuffers
        core::recordTriangleCommands( commandResources.buffers, renderPass, framebuffers.data(), framebuffers.size(), Display.extent, graphicsPipeline );

        // Reset current frame to avoid issues with sync objects
        currentFrame = 0;
        resize = false;
      }

      // Draw one frame; handle OutOfDate/Suboptimal by recreating immediately
      uint32_t drawResult = 0;
      try
      {
        core::drawFrame( deviceBundle.device, swapchain.swapchain, deviceBundle.graphicsQueue, deviceBundle.presentQueue, commandResources.buffers, syncObjects, currentFrame );
      }
      catch ( std::exception const & err )
      {
        isDebug(std::println( "drawFrame exception: {}", err.what() ));
        resize = true;
      }
    }

    // std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
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
