#include "buffer.hpp"

#include <cstring>

namespace offload
{

  BufferAllocation createVertexBuffer( VmaAllocator allocator, const std::array<Vertex, 3> & vertices )
  {
    VkDeviceSize bufferSize = sizeof( vertices );

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size               = bufferSize;
    bufferInfo.usage              = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags                   = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    BufferAllocation result;
    vmaCreateBuffer( allocator, &bufferInfo, &allocInfo, &result.buffer, &result.allocation, &result.info );

    // Copy vertex data to buffer (already mapped)
    memcpy( result.info.pMappedData, vertices.data(), static_cast<size_t>( bufferSize ) );

    return result;
  }

}  // namespace offload
