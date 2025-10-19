#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

namespace offload
{

  VmaAllocator createAllocator( vk::raii::Instance const & instance, vk::raii::PhysicalDevice const & physicalDevice, vk::raii::Device const & device );

}  // namespace offload
