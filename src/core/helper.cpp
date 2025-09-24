#include "./helper.hpp"

// #include <vulkan/vulkan_raii.hpp>`

namespace core
{
  VKAPI_ATTR vk::Bool32 VKAPI_CALL debugUtilsMessengerCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT messageTypes, const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData, void * /*pUserData*/ )
  {
    std::println( "validation layer (severity: {}): {}", vk::to_string( messageSeverity ), pCallbackData->pMessage );
    return vk::False;
  }

  vk::InstanceCreateInfo createInstanceCreateInfo( const std::string & appName, const std::string & engineName, std::vector<const char *> layers, std::vector<const char *> extensions )
  {
    vk::ApplicationInfo applicationInfo(
      appName.c_str(),
      1,  // application version
      engineName.c_str(),
      1,  // engine version
      VK_API_VERSION_1_3 );

    vk::InstanceCreateInfo instanceCreateInfo( {}, &applicationInfo, layers, extensions );

    return instanceCreateInfo;
  }

  // Helper function to create a DebugUtilsMessengerCreateInfoEXT with default settings and callback
  vk::DebugUtilsMessengerCreateInfoEXT createDebugUtilsMessengerCreateInfo()
  {
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {};

    debugUtilsMessengerCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

    debugUtilsMessengerCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

    debugUtilsMessengerCreateInfo.pfnUserCallback = core::debugUtilsMessengerCallback;
    return debugUtilsMessengerCreateInfo;
  }

  // Selects the best physical device, prioritizing discrete GPUs.
  // Returns the index of the selected device in the input vector, or -1 if none found.
  // Selects the best physical device, prioritizing discrete GPUs.
  // Returns a vk::raii::PhysicalDevice, or a default-constructed (invalid) one if none found.
  vk::raii::PhysicalDevice selectPhysicalDevice( const vk::raii::PhysicalDevices & devices )
  {
    if ( devices.empty() )
    {
      throw std::runtime_error( "No Vulkan physical devices found." );
    }

    // First, try to find a discrete GPU
    for ( const auto & device : devices )
    {
      vk::PhysicalDeviceProperties props = device.getProperties();
      if ( props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
      {
        return device;
      }
    }

    // If no discrete GPU, just return the first device
    return devices.front();
  }

  std::vector<const char *> getInstanceExtensions()
  {
    std::vector<const char *> extensions;

    isDebug( extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME ); );

    extensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
    extensions.push_back( VK_KHR_ANDROID_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_METAL_EXT )
    extensions.push_back( VK_EXT_METAL_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_VI_NN )
    extensions.push_back( VK_NN_VI_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_WAYLAND_KHR )
    extensions.push_back( VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_WIN32_KHR )
    extensions.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_XCB_KHR )
    extensions.push_back( VK_KHR_XCB_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_XLIB_KHR )
    extensions.push_back( VK_KHR_XLIB_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_XLIB_XRANDR_EXT )
    extensions.push_back( VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME );
#endif
    return extensions;
  }

  SurfaceData::SurfaceData( vk::raii::Instance const & instance, std::string const & windowName, vk::Extent2D const & extent_ ) : extent( extent_ ), window( nullptr ), name( windowName ), surface( nullptr )
  {
    if ( !glfwInit() )
    {
      throw std::runtime_error( "Failed to initialize GLFW!" );
    }
    glfwSetErrorCallback( []( int error, const char * msg ) { std::cerr << "glfw: " << "(" << error << ") " << msg << std::endl; } );

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    window = glfwCreateWindow( extent.width, extent.height, windowName.c_str(), nullptr, nullptr );
    if ( !window )
    {
      glfwTerminate();
      throw std::runtime_error( "Failed to create GLFW window!" );
    }

    VkSurfaceKHR _surface;
    VkResult     err = glfwCreateWindowSurface( *instance, window, nullptr, &_surface );
    if ( err != VK_SUCCESS )
    {
      glfwDestroyWindow( window );
      glfwTerminate();
      throw std::runtime_error( "Failed to create window surface!" );
    }
    surface = vk::raii::SurfaceKHR( instance, _surface );
  }

  SurfaceData::~SurfaceData()
  {
    if ( window )
    {
      glfwDestroyWindow( window );
      window = nullptr;
    }
    glfwTerminate();
  }

}  // namespace core
