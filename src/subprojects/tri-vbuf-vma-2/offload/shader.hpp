#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace offload
{

  struct ShaderObjects
  {
    vk::raii::ShaderEXT vertShader;
    vk::raii::ShaderEXT fragShader;
  };

  vk::raii::PipelineLayout createPipelineLayout( vk::raii::Device const & device );

  ShaderObjects createShaderObjects( vk::raii::Device const & device, const vk::PushConstantRange & pushConstantRange );

}  // namespace offload
