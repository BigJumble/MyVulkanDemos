#pragma once
#define GLFW_INCLUDE_NONE
#include "setup.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

// clang-format on

namespace global
{
  namespace obj
  {
    inline vk::raii::Context         context;
    inline vk::raii::Instance        instance        = nullptr;
    inline vk::raii::PhysicalDevices physicalDevices = nullptr;
    inline vk::raii::PhysicalDevice  physicalDevice  = nullptr;

    inline std::unique_ptr<GLFWwindow, void (*)(GLFWwindow *)> window{nullptr, core::glfwDestructor};
    inline vk::raii::SurfaceKHR surface = nullptr;

    inline core::QueueFamilyIndices queueFamilyIndices;

    inline vk::raii::Device device        = nullptr;
    inline vk::raii::Queue  graphicsQueue = nullptr;
    inline vk::raii::Queue  presentQueue  = nullptr;
    inline vk::raii::Queue  computeQueue  = nullptr;

    inline core::SwapchainBundle swapchainBundle;


  }  // namespace obj
}  // namespace global
