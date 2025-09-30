#include "helper.hpp"
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

    // Create a pipeline layout (no descriptors for this simple triangle)
    vk::raii::PipelineLayout pipelineLayout = core::createPipelineLayout( deviceBundle.device );

    // Create the graphics pipeline using dynamic rendering
    vk::raii::Pipeline graphicsPipeline = core::createGraphicsPipeline( deviceBundle.device, pipelineLayout, swapchain.extent, vertShaderModule, fragShaderModule, swapchain.imageFormat );

    // Create command pool and buffers (one per swapchain image)
    core::CommandResources commandResources = core::createCommandResources( deviceBundle.device, deviceBundle.indices.graphicsFamily, swapchain.imageViews.size() );

    // Record commands to draw the triangle using dynamic rendering
    core::recordTriangleCommands( commandResources.buffers, swapchain.imageViews, swapchain.extent, graphicsPipeline );

    // Create synchronization objects for 2 frames in flight
    core::SyncObjects syncObjects = core::createSyncObjects( deviceBundle.device, 2 );

    // Main render loop (draw a few frames, then exit)
    size_t currentFrame = 0;
    for ( int frame = 0; frame < 100; ++frame )
    {
      // Draw one frame
      try
      {
        core::drawFrame( deviceBundle.device, swapchain.swapchain, deviceBundle.graphicsQueue, deviceBundle.presentQueue, commandResources.buffers, syncObjects, currentFrame );
      }
      catch ( vk::OutOfDateKHRError const & )
      {
        // ignore in this short demo loop
        break;
      }
      catch ( std::exception const & err )
      {
        std::println( "drawFrame exception: {}", err.what() );
        break;
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
