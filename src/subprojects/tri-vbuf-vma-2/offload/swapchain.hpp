#pragma once

#include "bootstrap.hpp"

namespace offload
{

  void framebufferResizeCallback( GLFWwindow * win, int, int );

  void recreateSwapchain(
    core::DisplayBundle &      displayBundle,
    vk::raii::PhysicalDevice & physicalDevice,
    core::DeviceBundle &       deviceBundle,
    core::SwapchainBundle &    swapchainBundle,
    core::QueueFamilyIndices & queueFamilyIndices );

}  // namespace offload
