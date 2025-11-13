#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

namespace eeng
{
  namespace raii
  {
    struct Allocator
    {
      VmaAllocator allocator = nullptr;

      // Default constructor
      Allocator() = default;

      // Normal constructor
      Allocator( const vk::raii::Instance & instance, const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::Device & device )
      {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_4;
        allocatorInfo.physicalDevice         = *physicalDevice;
        allocatorInfo.device                 = *device;
        allocatorInfo.instance               = *instance;

        vmaCreateAllocator( &allocatorInfo, &allocator );
      }

      // Move constructor
      Allocator( Allocator && other ) noexcept
      {
        allocator       = other.allocator;
        other.allocator = nullptr;
      }

      // Move assignment operator
      Allocator & operator=( Allocator && other ) noexcept
      {
        if ( this != &other )
        {
          clear();
          allocator       = other.allocator;
          other.allocator = nullptr;
        }
        return *this;
      }

      // Delete copy constructor and copy assignment operator
      Allocator( const Allocator & )             = delete;
      Allocator & operator=( const Allocator & ) = delete;

      operator VmaAllocator() const
      {
        return allocator;
      }

      ~Allocator()
      {
        clear();
      }

      void clear()
      {
        if ( allocator )
        {
          vmaDestroyAllocator( allocator );
          allocator = nullptr;
        }
      }
    };
  }  // namespace raii
}  // namespace eeng
