#include "./helper.hpp"




VKAPI_ATTR vk::Bool32 VKAPI_CALL debugUtilsMessengerCallback( vk::DebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                                                              vk::DebugUtilsMessageTypeFlagsEXT              messageTypes,
                                                              const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData,
                                                              void * /*pUserData*/ )
{
  std::cerr << vk::to_string( messageSeverity ) << ": " << vk::to_string( messageTypes ) << ":\n";
  std::cerr << std::string( "\t" ) << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
  std::cerr << std::string( "\t" ) << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
  std::cerr << std::string( "\t" ) << "message         = <" << pCallbackData->pMessage << ">\n";
  if ( 0 < pCallbackData->queueLabelCount )
  {
    std::cerr << std::string( "\t" ) << "Queue Labels:\n";
    for ( uint32_t i = 0; i < pCallbackData->queueLabelCount; i++ )
    {
      std::cerr << std::string( "\t\t" ) << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
    }
  }
  if ( 0 < pCallbackData->cmdBufLabelCount )
  {
    std::cerr << std::string( "\t" ) << "CommandBuffer Labels:\n";
    for ( uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++ )
    {
      std::cerr << std::string( "\t\t" ) << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
    }
  }
  if ( 0 < pCallbackData->objectCount )
  {
    std::cerr << std::string( "\t" ) << "Objects:\n";
    for ( uint32_t i = 0; i < pCallbackData->objectCount; i++ )
    {
      std::cerr << std::string( "\t\t" ) << "Object " << i << "\n";
      std::cerr << std::string( "\t\t\t" ) << "objectType   = " << vk::to_string( pCallbackData->pObjects[i].objectType ) << "\n";
      std::cerr << std::string( "\t\t\t" ) << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
      if ( pCallbackData->pObjects[i].pObjectName )
      {
        std::cerr << std::string( "\t\t\t" ) << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
      }
    }
  }
  return vk::False;
}
