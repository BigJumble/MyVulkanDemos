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

    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
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
