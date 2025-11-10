#pragma once
#include <GLFW/glfw3.h>
#include <optional>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

namespace core
{
  // GLFW-based key and mouse enumeration
  enum class Key
  {
    W            = GLFW_KEY_W,
    A            = GLFW_KEY_A,
    S            = GLFW_KEY_S,
    D            = GLFW_KEY_D,
    LeftShift    = GLFW_KEY_LEFT_SHIFT,
    RightShift   = GLFW_KEY_RIGHT_SHIFT,
    LeftControl  = GLFW_KEY_LEFT_CONTROL,
    RightControl = GLFW_KEY_RIGHT_CONTROL,
    Space        = GLFW_KEY_SPACE,
    Escape       = GLFW_KEY_ESCAPE,
    F11          = GLFW_KEY_F11,
    MouseLeft    = GLFW_MOUSE_BUTTON_LEFT,
    MouseRight   = GLFW_MOUSE_BUTTON_RIGHT,
    MouseMiddle  = GLFW_MOUSE_BUTTON_MIDDLE
  };

  struct QueueFamilyIndices
  {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;
  };

  struct SwapchainSupportDetails
  {
    vk::SurfaceCapabilitiesKHR        capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR>   presentModes;
  };

  struct SwapchainBundle
  {
    vk::raii::SwapchainKHR           swapchain = nullptr;
    vk::Format                       imageFormat;
    vk::Extent2D                     extent;
    std::vector<vk::Image>           images;
    std::vector<vk::raii::ImageView> imageViews;
  };

  struct Texture
  {
    VmaAllocation allocation;
    vk::Image     image;
    vk::ImageView imageView;
    vk::Sampler   sampler;
    vk::Format    format;
    vk::Extent2D  extent;
  };

  struct Buffer
  {
    VkBuffer          buffer     = VK_NULL_HANDLE;
    VmaAllocation     allocation = nullptr;
    VmaAllocationInfo allocationInfo{};  // may contain mapped pointer for host visible buffers
    VkDeviceSize      size = 0;
  };

  struct FrameInFlight
  {
    vk::raii::Semaphore imageAvailable = nullptr;
    vk::raii::Semaphore renderFinished = nullptr;
    vk::raii::Fence     presentFence   = nullptr;
  };
}  // namespace core
