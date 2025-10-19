#pragma once
#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace core
{



  // Result of shader compilation using shaderc, return SPIR-V code or error messages
  struct ShaderCompileResult
  {
    bool                       success = false;
    std::vector<uint32_t>      spirv;         // Compiled SPIR-V code
    std::string                errorMessage;  // Error/warning messages
    size_t                     numWarnings = 0;
    size_t                     numErrors   = 0;
    shaderc_compilation_status status      = shaderc_compilation_status_success;
  };

  // Compilation options for shader compilation
  struct ShaderCompileOptions
  {
    bool                       optimize          = true;                                    // Enable optimization
    bool                       generateDebugInfo = false;                                   // Generate debug info
    shaderc_optimization_level optimizationLevel = shaderc_optimization_level_performance;  // Optimization level
    std::vector<std::string>   macroDefinitions;                                            // Preprocessor macros (format: "NAME=VALUE" or "NAME")
    std::string                entryPoint = "main";                                         // Shader entry point
  };

  /**
   * @brief Compiles raw shader source code to SPIR-V
   * @param source The shader source code (GLSL)
   * @param stage The shader stage (vertex, fragment, compute, etc.)
   * @param options Compilation options
   * @param sourceName Optional name for the shader source (used in error messages)
   * @return ShaderCompileResult containing SPIR-V code or error messages
   */
  [[nodiscard]] ShaderCompileResult
    compileShaderFromSource( const std::string & source, ShaderStage stage, const ShaderCompileOptions & options = {}, const std::string & sourceName = "shader" );

  /**
   * @brief Compiles a shader file to SPIR-V
   * @param filePath Path to the shader file
   * @param stage The shader stage (vertex, fragment, compute, etc.)
   * @param options Compilation options
   * @return ShaderCompileResult containing SPIR-V code or error messages
   */
  [[nodiscard]] ShaderCompileResult compileShaderFromFile( const std::string & filePath, ShaderStage stage, const ShaderCompileOptions & options = {} );

  // Reflected shader data from SPIRV-Reflect
  struct ShaderReflectionData
  {
    vk::ShaderStageFlagBits shaderStage;
    std::string             entryPoint;

    // Descriptor set information
    struct DescriptorSetLayoutData
    {
      uint32_t                                    setNumber;
      std::vector<vk::DescriptorSetLayoutBinding> bindings;
    };
    std::vector<DescriptorSetLayoutData> descriptorSets;

    // Push constant information
    std::vector<vk::PushConstantRange> pushConstants;

    // Vertex input attributes (for vertex shaders)
    std::vector<vk::VertexInputAttributeDescription> vertexInputAttributes;

    // Flags indicating what data is present
    struct DataFlags
    {
      bool hasDescriptorSets      = false;
      bool hasPushConstants       = false;
      bool hasVertexInputs        = false;
    } dataFlags;
  };

  // Complete shader object result with VkShaderEXT and reflection data
  struct ShaderObjectResult
  {
    bool                      success = false;
    std::string               errorMessage;
    
    VkShaderEXT               shaderEXT = VK_NULL_HANDLE;  // Raw handle, needs manual cleanup
    ShaderReflectionData      reflectionData;
    std::vector<uint32_t>     spirv;  // Keep SPIR-V for debugging/caching
  };

  /**
   * @brief Creates a shader object (VkShaderEXT) from shader source code
   * 
   * This function:
   * 1. Compiles shader source to SPIR-V using shaderc
   * 2. Reflects the SPIR-V to extract descriptor sets, push constants, etc.
   * 3. Creates a VkShaderEXT using VK_EXT_shader_object
   * 
   * @param device The Vulkan device
   * @param source The shader source code (GLSL)
   * @param stage The shader stage
   * @param options Compilation options
   * @param sourceName Optional name for error messages
   * @return ShaderObjectResult containing the shader object and reflection data
   */
  [[nodiscard]] ShaderObjectResult createShaderObject(
    VkDevice                     device,
    const std::string &          source,
    ShaderStage                  stage,
    const ShaderCompileOptions & options    = {},
    const std::string &          sourceName = "shader" );

  /**
   * @brief Creates a shader object from a shader file
   * 
   * @param device The Vulkan device
   * @param filePath Path to the shader file
   * @param stage The shader stage
   * @param options Compilation options
   * @return ShaderObjectResult containing the shader object and reflection data
   */
  [[nodiscard]] ShaderObjectResult createShaderObjectFromFile(
    VkDevice                     device,
    const std::string &          filePath,
    ShaderStage                  stage,
    const ShaderCompileOptions & options = {} );

  /**
   * @brief Destroys a shader object created with createShaderObject
   * 
   * @param device The Vulkan device
   * @param shaderEXT The shader object to destroy
   */
  void destroyShaderObject( VkDevice device, VkShaderEXT shaderEXT );


}  // namespace core
