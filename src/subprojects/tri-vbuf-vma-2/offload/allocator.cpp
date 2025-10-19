#include "allocator.hpp"

namespace offload
{

  VmaAllocator createAllocator( vk::raii::Instance const & instance, vk::raii::PhysicalDevice const & physicalDevice, vk::raii::Device const & device )
  {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_4;
    allocatorInfo.physicalDevice         = *physicalDevice;
    allocatorInfo.device                 = *device;
    allocatorInfo.instance               = *instance;

    VmaAllocator allocator;
    vmaCreateAllocator( &allocatorInfo, &allocator );

    return allocator;
  }

}  // namespace offload
