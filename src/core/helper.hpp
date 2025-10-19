#pragma once

#include <string>
#include <vector>
// #include <shaderc/shaderc.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace core
{

  namespace help
  {

    // Map a shader name or extension to a Vulkan shader stage flag
    // Example usage: vkStageFromShaderName("myshader.vert") returns vk::ShaderStageFlagBits::eVertex
    inline vk::ShaderStageFlagBits vkStageFromShaderName( const std::string & name )
    {
      // Try to extract file extension from the name
      // Support: .vert .frag .comp .geom .tesc .tese .rgen .rchit .rmiss .rahit .rcall
      auto pos = name.find_last_of( '.' );
      if ( pos == std::string::npos || pos == name.length() - 1 )
        throw std::invalid_argument( "Shader name has no extension: " + name );

      std::string ext = name.substr( pos + 1 );
      if ( ext == "vert" )
        return vk::ShaderStageFlagBits::eVertex;
      if ( ext == "frag" )
        return vk::ShaderStageFlagBits::eFragment;
      if ( ext == "comp" )
        return vk::ShaderStageFlagBits::eCompute;
      if ( ext == "geom" )
        return vk::ShaderStageFlagBits::eGeometry;
      if ( ext == "tesc" )
        return vk::ShaderStageFlagBits::eTessellationControl;
      if ( ext == "tese" )
        return vk::ShaderStageFlagBits::eTessellationEvaluation;
      if ( ext == "rgen" )
        return vk::ShaderStageFlagBits::eRaygenKHR;
      if ( ext == "rchit" )
        return vk::ShaderStageFlagBits::eClosestHitKHR;
      if ( ext == "rmiss" )
        return vk::ShaderStageFlagBits::eMissKHR;
      if ( ext == "rahit" )
        return vk::ShaderStageFlagBits::eAnyHitKHR;
      if ( ext == "rcall" )
        return vk::ShaderStageFlagBits::eCallableKHR;

      throw std::invalid_argument( "Unknown shader extension: " + ext );
    }

    /**
     * @brief Reads shader source code from a file
     * @param shaderPath Path to the shader source file
     * @return Shader source code as string
     */
    std::string readShaderSource( const std::string & shaderPath );

    /**
     * @brief Compiles a shader file and returns the SPIR-V code
     * @param shaderName Name of the shader file (e.g., "triangle.vert")
     * @return Vector of SPIR-V code
     */
    std::vector<uint32_t> compileShader( const std::string & shaderName );

    /**
     * @brief Gets shader code by either reading from /compiled or compiling from /shaders
     * @param shaderName The name of the shader (e.g., "triangle.vert")
     * @return Vector of SPIR-V code
     */
    std::vector<uint32_t> getShaderCode( const std::string & shaderName );

  }  // namespace help

}  // namespace core
