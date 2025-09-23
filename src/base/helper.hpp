#pragma once


#include <GLFW/glfw3.h>
#include <chrono>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <print>
#include <string>
#include <thread>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
// int add( int a, int b );



using Transfrom = glm::mat4x4;

VKAPI_ATTR vk::Bool32 VKAPI_CALL debugUtilsMessengerCallback( vk::DebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                                                              vk::DebugUtilsMessageTypeFlagsEXT              messageTypes,
                                                              const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData,
                                                              void * /*pUserData*/ );

