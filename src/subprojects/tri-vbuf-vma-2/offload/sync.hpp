#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace offload
{

  struct FrameSyncObjects
  {
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence>     presentFences;
  };

  FrameSyncObjects createFrameSyncObjects( vk::raii::Device const & device, size_t maxFramesInFlight );

}  // namespace offload
