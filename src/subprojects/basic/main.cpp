
#include "helper.hpp"

static std::string AppName    = "01_InitInstance";
static std::string EngineName = "Vulkan.hpp";

int main()
{
  /* VULKAN_HPP_KEY_START */

  try
  {
    // the very beginning: instantiate a context
    vk::raii::Context context;
    // initialize the vk::ApplicationInfo structure
    vk::ApplicationInfo applicationInfo( AppName.c_str(), 1, EngineName.c_str(), 1, VK_API_VERSION_1_3 );

    // initialize the vk::InstanceCreateInfo
    std::vector<const char *> layers{ "VK_LAYER_KHRONOS_validation" };
    std::vector<const char *> extensions{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    vk::InstanceCreateInfo    instanceCreateInfo( {}, &applicationInfo, {}, extensions );

    // create an Instance
    vk::raii::Instance instance( context, instanceCreateInfo );

    // #if !defined( NDEBUG )
    // Debug messenger create info
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {};
    debugUtilsMessengerCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debugUtilsMessengerCreateInfo.messageType =
      vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

    // Assign the callback function directly
    debugUtilsMessengerCreateInfo.pfnUserCallback = debugUtilsMessengerCallback;

    // Create the debug messenger
    vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger( instance, debugUtilsMessengerCreateInfo );
    // #endif

    // enumerate the physicalDevices
    vk::raii::PhysicalDevices physicalDevices( instance );

    for ( auto const & pdh : physicalDevices )
    {
      vk::PhysicalDeviceProperties properties = pdh.getProperties();

      std::cout << "apiVersion: ";
      std::cout << ( ( properties.apiVersion >> 22 ) & 0xfff ) << '.';  // Major.
      std::cout << ( ( properties.apiVersion >> 12 ) & 0x3ff ) << '.';  // Minor.
      std::cout << ( properties.apiVersion & 0xfff );                   // Patch.
      std::cout << '\n';

      std::cout << "driverVersion: " << properties.driverVersion << '\n';

      std::cout << std::showbase << std::internal << std::setfill( '0' ) << std::hex;
      std::cout << "vendorId: " << std::setw( 6 ) << properties.vendorID << '\n';
      std::cout << "deviceId: " << std::setw( 6 ) << properties.deviceID << '\n';
      std::cout << std::noshowbase << std::right << std::setfill( ' ' ) << std::dec;

      std::cout << "deviceType: " << vk::to_string( properties.deviceType ) << "\n";

      std::cout << "deviceName: " << properties.deviceName << '\n';

      // std::cout << "pipelineCacheUUID: " << vk::su::UUID( properties.pipelineCacheUUID ) << "\n\n";
    }
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
