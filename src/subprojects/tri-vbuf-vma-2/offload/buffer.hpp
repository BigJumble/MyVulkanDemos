#pragma once

#include "types.hpp"

#include <array>
#include <vk_mem_alloc.h>

namespace offload
{

  struct BufferAllocation
  {
    VkBuffer          buffer;
    VmaAllocation     allocation;
    VmaAllocationInfo info;
  };

  BufferAllocation createVertexBuffer( VmaAllocator allocator, const std::array<Vertex, 3> & vertices );

}  // namespace offload
