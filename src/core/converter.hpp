#pragma once
#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>
#include <vulkan/vulkan_raii.hpp>

namespace core
{
  namespace to
  {

    // Convert Vulkan shader stage to Shaderc shader kind
    constexpr shaderc_shader_kind shadercKind( vk::ShaderStageFlagBits stage )
    {
      switch ( stage )
      {
        case vk::ShaderStageFlagBits::eVertex                : return shaderc_vertex_shader;
        case vk::ShaderStageFlagBits::eFragment              : return shaderc_fragment_shader;
        case vk::ShaderStageFlagBits::eCompute               : return shaderc_compute_shader;
        case vk::ShaderStageFlagBits::eGeometry              : return shaderc_geometry_shader;
        case vk::ShaderStageFlagBits::eTessellationControl   : return shaderc_tess_control_shader;
        case vk::ShaderStageFlagBits::eTessellationEvaluation: return shaderc_tess_evaluation_shader;
        case vk::ShaderStageFlagBits::eRaygenKHR             : return shaderc_raygen_shader;
        case vk::ShaderStageFlagBits::eAnyHitKHR             : return shaderc_anyhit_shader;
        case vk::ShaderStageFlagBits::eClosestHitKHR         : return shaderc_closesthit_shader;
        case vk::ShaderStageFlagBits::eMissKHR               : return shaderc_miss_shader;
        case vk::ShaderStageFlagBits::eIntersectionKHR       : return shaderc_intersection_shader;
        case vk::ShaderStageFlagBits::eCallableKHR           : return shaderc_callable_shader;
        default                                              : throw std::invalid_argument( "Unsupported shader stage" );
      }
    }

    // Convert Shaderc shader kind to Vulkan shader stage
    constexpr vk::ShaderStageFlagBits vulkanStage( shaderc_shader_kind kind )
    {
      switch ( kind )
      {
        case shaderc_vertex_shader         : return vk::ShaderStageFlagBits::eVertex;
        case shaderc_fragment_shader       : return vk::ShaderStageFlagBits::eFragment;
        case shaderc_compute_shader        : return vk::ShaderStageFlagBits::eCompute;
        case shaderc_geometry_shader       : return vk::ShaderStageFlagBits::eGeometry;
        case shaderc_tess_control_shader   : return vk::ShaderStageFlagBits::eTessellationControl;
        case shaderc_tess_evaluation_shader: return vk::ShaderStageFlagBits::eTessellationEvaluation;
        case shaderc_raygen_shader         : return vk::ShaderStageFlagBits::eRaygenKHR;
        case shaderc_anyhit_shader         : return vk::ShaderStageFlagBits::eAnyHitKHR;
        case shaderc_closesthit_shader     : return vk::ShaderStageFlagBits::eClosestHitKHR;
        case shaderc_miss_shader           : return vk::ShaderStageFlagBits::eMissKHR;
        case shaderc_intersection_shader   : return vk::ShaderStageFlagBits::eIntersectionKHR;
        case shaderc_callable_shader       : return vk::ShaderStageFlagBits::eCallableKHR;
        default                            : throw std::invalid_argument( "Unsupported shader kind" );
      }
    }

    inline SpvReflectShaderStageFlagBits spvStage( vk::ShaderStageFlagBits stage )
    {
      return static_cast<SpvReflectShaderStageFlagBits>( stage );
    }

    // Convert Vulkan shader stage to SPIRV-Reflect shader stage
    constexpr SpvReflectShaderStageFlagBits reflectStage( vk::ShaderStageFlagBits stage )
    {
      // return static_cast<SpvReflectShaderStageFlagBits>(stage); // lol just use this
      // AI slop for the win!
      // I mean - error checking at default
      switch ( stage )
      {
        case vk::ShaderStageFlagBits::eVertex                : return SPV_REFLECT_SHADER_STAGE_VERTEX_BIT;
        case vk::ShaderStageFlagBits::eTessellationControl   : return SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case vk::ShaderStageFlagBits::eTessellationEvaluation: return SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case vk::ShaderStageFlagBits::eGeometry              : return SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT;
        case vk::ShaderStageFlagBits::eFragment              : return SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT;
        case vk::ShaderStageFlagBits::eCompute               : return SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT;
        case vk::ShaderStageFlagBits::eRaygenKHR             : return SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR;
        case vk::ShaderStageFlagBits::eAnyHitKHR             : return SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR;
        case vk::ShaderStageFlagBits::eClosestHitKHR         : return SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        case vk::ShaderStageFlagBits::eMissKHR               : return SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR;
        case vk::ShaderStageFlagBits::eIntersectionKHR       : return SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR;
        case vk::ShaderStageFlagBits::eCallableKHR           : return SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR;
        case vk::ShaderStageFlagBits::eMeshEXT               : return SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT;
        case vk::ShaderStageFlagBits::eTaskEXT               : return SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT;
        default                                              : throw std::invalid_argument( "Unsupported shader stage" );
      }
    }

    // Convert SPIRV-Reflect shader stage to Vulkan shader stage
    constexpr vk::ShaderStageFlagBits vulkanStage( SpvReflectShaderStageFlagBits stage )
    {
      switch ( stage )
      {
        case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT                 : return vk::ShaderStageFlagBits::eVertex;
        case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT   : return vk::ShaderStageFlagBits::eTessellationControl;
        case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return vk::ShaderStageFlagBits::eTessellationEvaluation;
        case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT               : return vk::ShaderStageFlagBits::eGeometry;
        case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT               : return vk::ShaderStageFlagBits::eFragment;
        case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT                : return vk::ShaderStageFlagBits::eCompute;
        case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR             : return vk::ShaderStageFlagBits::eRaygenKHR;
        case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR            : return vk::ShaderStageFlagBits::eAnyHitKHR;
        case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR        : return vk::ShaderStageFlagBits::eClosestHitKHR;
        case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR               : return vk::ShaderStageFlagBits::eMissKHR;
        case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR       : return vk::ShaderStageFlagBits::eIntersectionKHR;
        case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR           : return vk::ShaderStageFlagBits::eCallableKHR;
        case SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT               : return vk::ShaderStageFlagBits::eTaskEXT;
        case SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT               : return vk::ShaderStageFlagBits::eMeshEXT;
        default                                                  : throw std::invalid_argument( "Unsupported shader stage" );
      }
    }

    // Convert SpvReflectFormat to vk::Format (constexpr)
    constexpr vk::Format vulkanFormat( SpvReflectFormat format )
    {
      switch ( format )
      {
        case SPV_REFLECT_FORMAT_UNDEFINED          : return vk::Format::eUndefined;
        case SPV_REFLECT_FORMAT_R32_UINT           : return vk::Format::eR32Uint;
        case SPV_REFLECT_FORMAT_R32_SINT           : return vk::Format::eR32Sint;
        case SPV_REFLECT_FORMAT_R32_SFLOAT         : return vk::Format::eR32Sfloat;
        case SPV_REFLECT_FORMAT_R32G32_UINT        : return vk::Format::eR32G32Uint;
        case SPV_REFLECT_FORMAT_R32G32_SINT        : return vk::Format::eR32G32Sint;
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT      : return vk::Format::eR32G32Sfloat;
        case SPV_REFLECT_FORMAT_R32G32B32_UINT     : return vk::Format::eR32G32B32Uint;
        case SPV_REFLECT_FORMAT_R32G32B32_SINT     : return vk::Format::eR32G32B32Sint;
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT   : return vk::Format::eR32G32B32Sfloat;
        case SPV_REFLECT_FORMAT_R32G32B32A32_UINT  : return vk::Format::eR32G32B32A32Uint;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SINT  : return vk::Format::eR32G32B32A32Sint;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return vk::Format::eR32G32B32A32Sfloat;
        case SPV_REFLECT_FORMAT_R64_UINT           : return vk::Format::eR64Uint;
        case SPV_REFLECT_FORMAT_R64_SINT           : return vk::Format::eR64Sint;
        case SPV_REFLECT_FORMAT_R64_SFLOAT         : return vk::Format::eR64Sfloat;
        case SPV_REFLECT_FORMAT_R64G64_UINT        : return vk::Format::eR64G64Uint;
        case SPV_REFLECT_FORMAT_R64G64_SINT        : return vk::Format::eR64G64Sint;
        case SPV_REFLECT_FORMAT_R64G64_SFLOAT      : return vk::Format::eR64G64Sfloat;
        case SPV_REFLECT_FORMAT_R64G64B64_UINT     : return vk::Format::eR64G64B64Uint;
        case SPV_REFLECT_FORMAT_R64G64B64_SINT     : return vk::Format::eR64G64B64Sint;
        case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT   : return vk::Format::eR64G64B64Sfloat;
        case SPV_REFLECT_FORMAT_R64G64B64A64_UINT  : return vk::Format::eR64G64B64A64Uint;
        case SPV_REFLECT_FORMAT_R64G64B64A64_SINT  : return vk::Format::eR64G64B64A64Sint;
        case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT: return vk::Format::eR64G64B64A64Sfloat;
        default                                    : throw std::invalid_argument( "Unsupported format" );
      }
    }

    // Convert vk::Format to SpvReflectFormat (constexpr)
    constexpr SpvReflectFormat spvFormat( vk::Format format )
    {
      // return static_cast<SpvReflectFormat>(format); // lol just use this
      switch ( format )
      {
        case vk::Format::eUndefined         : return SPV_REFLECT_FORMAT_UNDEFINED;
        case vk::Format::eR32Uint           : return SPV_REFLECT_FORMAT_R32_UINT;
        case vk::Format::eR32Sint           : return SPV_REFLECT_FORMAT_R32_SINT;
        case vk::Format::eR32Sfloat         : return SPV_REFLECT_FORMAT_R32_SFLOAT;
        case vk::Format::eR32G32Uint        : return SPV_REFLECT_FORMAT_R32G32_UINT;
        case vk::Format::eR32G32Sint        : return SPV_REFLECT_FORMAT_R32G32_SINT;
        case vk::Format::eR32G32Sfloat      : return SPV_REFLECT_FORMAT_R32G32_SFLOAT;
        case vk::Format::eR32G32B32Uint     : return SPV_REFLECT_FORMAT_R32G32B32_UINT;
        case vk::Format::eR32G32B32Sint     : return SPV_REFLECT_FORMAT_R32G32B32_SINT;
        case vk::Format::eR32G32B32Sfloat   : return SPV_REFLECT_FORMAT_R32G32B32_SFLOAT;
        case vk::Format::eR32G32B32A32Uint  : return SPV_REFLECT_FORMAT_R32G32B32A32_UINT;
        case vk::Format::eR32G32B32A32Sint  : return SPV_REFLECT_FORMAT_R32G32B32A32_SINT;
        case vk::Format::eR32G32B32A32Sfloat: return SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT;
        case vk::Format::eR64Uint           : return SPV_REFLECT_FORMAT_R64_UINT;
        case vk::Format::eR64Sint           : return SPV_REFLECT_FORMAT_R64_SINT;
        case vk::Format::eR64Sfloat         : return SPV_REFLECT_FORMAT_R64_SFLOAT;
        case vk::Format::eR64G64Uint        : return SPV_REFLECT_FORMAT_R64G64_UINT;
        case vk::Format::eR64G64Sint        : return SPV_REFLECT_FORMAT_R64G64_SINT;
        case vk::Format::eR64G64Sfloat      : return SPV_REFLECT_FORMAT_R64G64_SFLOAT;
        case vk::Format::eR64G64B64Uint     : return SPV_REFLECT_FORMAT_R64G64B64_UINT;
        case vk::Format::eR64G64B64Sint     : return SPV_REFLECT_FORMAT_R64G64B64_SINT;
        case vk::Format::eR64G64B64Sfloat   : return SPV_REFLECT_FORMAT_R64G64B64_SFLOAT;
        case vk::Format::eR64G64B64A64Uint  : return SPV_REFLECT_FORMAT_R64G64B64A64_UINT;
        case vk::Format::eR64G64B64A64Sint  : return SPV_REFLECT_FORMAT_R64G64B64A64_SINT;
        case vk::Format::eR64G64B64A64Sfloat: return SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT;
        default                             : throw std::invalid_argument( "Unsupported format" );
      }
    }

    // Constexpr conversion functions between SpvReflectDescriptorType and vk::DescriptorType
    constexpr vk::DescriptorType vulkanDescriptorType( SpvReflectDescriptorType type )
    {
      switch ( type )
      {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER                   : return vk::DescriptorType::eSampler;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER    : return vk::DescriptorType::eCombinedImageSampler;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE             : return vk::DescriptorType::eSampledImage;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE             : return vk::DescriptorType::eStorageImage;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER      : return vk::DescriptorType::eUniformTexelBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER      : return vk::DescriptorType::eStorageTexelBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER            : return vk::DescriptorType::eUniformBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER            : return vk::DescriptorType::eStorageBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC    : return vk::DescriptorType::eUniformBufferDynamic;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC    : return vk::DescriptorType::eStorageBufferDynamic;
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT          : return vk::DescriptorType::eInputAttachment;
        case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return vk::DescriptorType::eAccelerationStructureKHR;
        default                                                    : throw std::invalid_argument( "Unsupported descriptor type" );
      }
    }

    constexpr SpvReflectDescriptorType spvDescriptorType( vk::DescriptorType type )
    {
      switch ( type )
      {
        case vk::DescriptorType::eSampler                 : return SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER;
        case vk::DescriptorType::eCombinedImageSampler    : return SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case vk::DescriptorType::eSampledImage            : return SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case vk::DescriptorType::eStorageImage            : return SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case vk::DescriptorType::eUniformTexelBuffer      : return SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case vk::DescriptorType::eStorageTexelBuffer      : return SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case vk::DescriptorType::eUniformBuffer           : return SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case vk::DescriptorType::eStorageBuffer           : return SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case vk::DescriptorType::eUniformBufferDynamic    : return SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case vk::DescriptorType::eStorageBufferDynamic    : return SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        case vk::DescriptorType::eInputAttachment         : return SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case vk::DescriptorType::eAccelerationStructureKHR: return SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        default                                           : throw std::invalid_argument( "Unsupported descriptor type" );
      }
    }
  }  // namespace to
}  // namespace core
