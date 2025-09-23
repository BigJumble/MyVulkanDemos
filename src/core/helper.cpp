#include "./helper.hpp"

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

}  // namespace core
