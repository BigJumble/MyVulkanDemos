#pragma once
#define GLFW_INCLUDE_NONE
#include "allocator.hpp"
#include "structs.hpp"
#include "window.hpp"

#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
// clang-format on

namespace eeng
{
  namespace obj
  {
    inline vk::raii::Context         context;
    inline vk::raii::Instance        instance        = nullptr;
    inline vk::raii::PhysicalDevices physicalDevices = nullptr;
    inline vk::raii::PhysicalDevice  physicalDevice  = nullptr;

    inline eeng::raii::Window window;
    // inline vk::raii::SurfaceKHR surface = nullptr;

    inline eeng::QueueFamilyIndices queueFamilyIndices;

    inline vk::raii::Device device        = nullptr;
    inline vk::raii::Queue  graphicsQueue = nullptr;
    inline vk::raii::Queue  presentQueue  = nullptr;
    inline vk::raii::Queue  computeQueue  = nullptr;

    inline eeng::SwapchainBundle swapchainBundle;

    inline eeng::raii::Allocator allocator;

    inline eeng::Texture depthTexture;
    inline eeng::Texture basicTargetTexture;

    inline vk::raii::DescriptorPool descriptorPool = nullptr;

    // inline core::raii::IMGUI IMGUI;

    inline vk::raii::CommandPool commandPool = nullptr;

    inline std::vector<eeng::FrameInFlight> frames;

    // Separate command buffers from frames-in-flight; singletons per slot
    inline vk::raii::CommandBuffers cmdScene   = nullptr;
    inline vk::raii::CommandBuffers cmdOverlay = nullptr;

    inline eeng::Buffer vertexBuffer;
    inline eeng::Buffer instanceBuffer;
  }  // namespace obj
}  // namespace eeng
