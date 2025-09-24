#include "helper.hpp"

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
    vk::raii::Instance instance( context, core::createInstanceCreateInfo( AppName, EngineName, {}, core::getInstanceExtensions() ) );

    isDebug( vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger( instance, core::createDebugUtilsMessengerCreateInfo() ); );

    vk::raii::PhysicalDevices physicalDevices( instance );

    vk::raii::PhysicalDevice physicalDevice = core::selectPhysicalDevice( physicalDevices );
    std::println( "device selected {}", (std::string)physicalDevice.getProperties().deviceName );

    core::SurfaceData Display( instance, "MyEngine", vk::Extent2D( 1280, 720 ) );

    // Get queue family properties for the selected physical device
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    std::println("Number of queue families: {}", queueFamilyProperties.size());
    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex  = UINT32_MAX;
    uint32_t computeQueueFamilyIndex  = UINT32_MAX;

    // Find graphics and present (surface) queue families
    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
    {
      const auto& qf = queueFamilyProperties[i];
      std::println("Queue Family {}:", i);
      std::println("  Queue Count: {}", qf.queueCount);
      std::println("  Flags: {}", vk::to_string(qf.queueFlags));
      std::println("  Timestamp Valid Bits: {}", qf.timestampValidBits);
      std::println("  Min Image Transfer Granularity: {}x{}x{}", qf.minImageTransferGranularity.width, qf.minImageTransferGranularity.height, qf.minImageTransferGranularity.depth);

      // Check for graphics support
      if ((qf.queueFlags & vk::QueueFlagBits::eGraphics) && graphicsQueueFamilyIndex == UINT32_MAX)
      {
        graphicsQueueFamilyIndex = i;
      }

      // Check for present (surface) support
      if (physicalDevice.getSurfaceSupportKHR(i, *Display.surface) && presentQueueFamilyIndex == UINT32_MAX)
      {
        presentQueueFamilyIndex = i;
      }

      // Check for compute support
      if ((qf.queueFlags & vk::QueueFlagBits::eCompute) && computeQueueFamilyIndex == UINT32_MAX)
      {
        computeQueueFamilyIndex = i;
      }
    }

    if (graphicsQueueFamilyIndex == UINT32_MAX)
    {
      throw std::runtime_error("No graphics queue family found.");
    }
    if (presentQueueFamilyIndex == UINT32_MAX)
    {
      throw std::runtime_error("No present (surface) queue family found.");
    }

    // // Prefer a dedicated compute queue if available; otherwise, fall back to graphics if it also supports compute
    // if (computeQueueFamilyIndex == UINT32_MAX)
    // {
    //   if (queueFamilyProperties[graphicsQueueFamilyIndex].queueFlags & vk::QueueFlagBits::eCompute)
    //   {
    //     computeQueueFamilyIndex = graphicsQueueFamilyIndex;
    //   }
    //   else
    //   {
    //     throw std::runtime_error("No compute queue family found.");
    //   }
    // }

    std::println("Graphics Queue Family Index: {}", graphicsQueueFamilyIndex);
    std::println("Present Queue Family Index: {}", presentQueueFamilyIndex);
    std::println("Compute Queue Family Index: {}", computeQueueFamilyIndex);


    // Prepare queue create infos
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {graphicsQueueFamilyIndex, presentQueueFamilyIndex, computeQueueFamilyIndex};
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
      queueCreateInfos.push_back(
        vk::DeviceQueueCreateInfo{}
          .setQueueFamilyIndex(queueFamily)
          .setQueueCount(1)
          .setPQueuePriorities(&queuePriority)
      );
    }

    vk::DeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.setQueueCreateInfos(queueCreateInfos);

    vk::raii::Device device(physicalDevice, deviceCreateInfo);

    // Retrieve queues (indices 0 assumed for single-queue requests)
    vk::raii::Queue graphicsQueue(device, graphicsQueueFamilyIndex, 0);
    vk::raii::Queue presentQueue(device, presentQueueFamilyIndex, 0);
    vk::raii::Queue computeQueue(device, computeQueueFamilyIndex, 0);

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
