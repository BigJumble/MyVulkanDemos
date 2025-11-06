#pragma once
#include <optional>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

namespace core
{

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
}  // namespace core
