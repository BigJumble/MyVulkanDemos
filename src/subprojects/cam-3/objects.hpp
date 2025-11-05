#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>
#include "setup.hpp"

// clang-format on

namespace global
{
  namespace obj
  {
    inline vk::raii::Context         context;
    inline vk::raii::Instance        instance        = nullptr;
    inline vk::raii::PhysicalDevices physicalDevices = nullptr;
    inline vk::raii::PhysicalDevice  physicalDevice  = nullptr;
    inline core::DisplayBundle       displayBundle;
    inline core::QueueFamilyIndices queueFamilyIndices;
  }  // namespace obj
}  // namespace global
