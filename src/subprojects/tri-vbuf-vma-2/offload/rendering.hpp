#pragma once

#include "bootstrap.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace offload
{

  void recordCommandBuffer(
    vk::raii::CommandBuffer &  cmd,
    vk::raii::ShaderEXT &      vertShaderObject,
    vk::raii::ShaderEXT &      fragShaderObject,
    core::SwapchainBundle &    swapchainBundle,
    uint32_t                   imageIndex,
    vk::raii::PipelineLayout & pipelineLayout,
    VkBuffer                   vertexBuffer );

}  // namespace offload
