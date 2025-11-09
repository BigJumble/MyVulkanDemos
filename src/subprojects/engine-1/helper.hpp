#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <sstream>
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>
#include <shaderc/shaderc.hpp>

// Debug macro
#if defined( DEBUG ) || !defined( NDEBUG )
#  define isDebug( code ) code
#else
#  define isDebug( code )
#endif

namespace core
{
  namespace to
  {
    // Converts Vulkan stage flag to shaderc shader kind
    inline shaderc_shader_kind shadercKind( vk::ShaderStageFlagBits stage )
    {
      switch ( stage )
      {
        case vk::ShaderStageFlagBits::eVertex: return shaderc_vertex_shader;
        case vk::ShaderStageFlagBits::eFragment: return shaderc_fragment_shader;
        case vk::ShaderStageFlagBits::eCompute: return shaderc_compute_shader;
        case vk::ShaderStageFlagBits::eGeometry: return shaderc_geometry_shader;
        case vk::ShaderStageFlagBits::eTessellationControl: return shaderc_tess_control_shader;
        case vk::ShaderStageFlagBits::eTessellationEvaluation: return shaderc_tess_evaluation_shader;
        case vk::ShaderStageFlagBits::eRaygenKHR: return shaderc_raygen_shader;
        case vk::ShaderStageFlagBits::eClosestHitKHR: return shaderc_closesthit_shader;
        case vk::ShaderStageFlagBits::eMissKHR: return shaderc_miss_shader;
        case vk::ShaderStageFlagBits::eAnyHitKHR: return shaderc_anyhit_shader;
        case vk::ShaderStageFlagBits::eCallableKHR: return shaderc_callable_shader;
        default: throw std::invalid_argument( "Unsupported Vulkan shader stage" );
      }
    }
  }  // namespace to

  namespace help
  {

    // Determine Vulkan shader stage from file extension
    inline vk::ShaderStageFlagBits vkStageFromShaderName( const std::string & name )
    {
      auto pos = name.find_last_of( '.' );
      if ( pos == std::string::npos || pos == name.length() - 1 )
        throw std::invalid_argument( "Shader name has no extension: " + name );

      std::string ext = name.substr( pos + 1 );
      if ( ext == "vert" ) return vk::ShaderStageFlagBits::eVertex;
      if ( ext == "frag" ) return vk::ShaderStageFlagBits::eFragment;
      if ( ext == "comp" ) return vk::ShaderStageFlagBits::eCompute;
      if ( ext == "geom" ) return vk::ShaderStageFlagBits::eGeometry;
      if ( ext == "tesc" ) return vk::ShaderStageFlagBits::eTessellationControl;
      if ( ext == "tese" ) return vk::ShaderStageFlagBits::eTessellationEvaluation;
      if ( ext == "rgen" ) return vk::ShaderStageFlagBits::eRaygenKHR;
      if ( ext == "rchit" ) return vk::ShaderStageFlagBits::eClosestHitKHR;
      if ( ext == "rmiss" ) return vk::ShaderStageFlagBits::eMissKHR;
      if ( ext == "rahit" ) return vk::ShaderStageFlagBits::eAnyHitKHR;
      if ( ext == "rcall" ) return vk::ShaderStageFlagBits::eCallableKHR;

      throw std::invalid_argument( "Unknown shader extension: " + ext );
    }

    // Read precompiled SPIR-V shader
    inline std::vector<uint32_t> readCompiledShader( const std::string & compiledPath )
    {
      std::ifstream compiledFile( compiledPath, std::ios::binary | std::ios::ate );
      if ( !compiledFile.is_open() ) return {};

      std::streamsize size = compiledFile.tellg();
      compiledFile.seekg( 0, std::ios::beg );

      if ( size % 4 == 0 && size > 0 )
      {
        std::vector<uint32_t> spirv( static_cast<size_t>( size ) / 4 );
        if ( compiledFile.read( reinterpret_cast<char *>( spirv.data() ), size ) )
        {
          return spirv;
        }
      }
      return {};
    }

    // Read shader GLSL source
    inline std::string readShaderSource( const std::string & shaderPath )
    {
      std::ifstream sourceFile( shaderPath );
      if ( !sourceFile.is_open() )
        throw std::runtime_error( "Failed to open shader source file: " + shaderPath );

      std::stringstream buffer;
      buffer << sourceFile.rdbuf();
      return buffer.str();
    }

    // Compile GLSL → SPIR-V using shaderc
    inline std::vector<uint32_t> compileShader( const std::string & shaderName )
    {
      shaderc_shader_kind kind =
        core::to::shadercKind( core::help::vkStageFromShaderName( shaderName ) );

      std::string sourcePath = "./shaders/" + shaderName;
      std::string source = readShaderSource( sourcePath );

      shaderc::Compiler compiler;
      shaderc::CompileOptions options;
      options.SetOptimizationLevel( shaderc_optimization_level_performance );

      shaderc::SpvCompilationResult result =
        compiler.CompileGlslToSpv( source, kind, shaderName.c_str(), options );

      if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
      {
        throw std::runtime_error(
          "Shader compilation failed for '" + sourcePath + "': " + result.GetErrorMessage() );
      }

      std::vector<uint32_t> spirv( result.cbegin(), result.cend() );

      // Save to ./compiled
      std::filesystem::create_directories( "./compiled" );
      std::string outputPath = "./compiled/" + shaderName + ".spv";

      std::ofstream outFile( outputPath, std::ios::binary );
      if ( !outFile.is_open() )
        throw std::runtime_error( "Failed to create output file: " + outputPath );

      outFile.write( reinterpret_cast<const char *>( spirv.data() ),
                     spirv.size() * sizeof( uint32_t ) );
      return spirv;
    }

    // Load or compile shader automatically
    inline std::vector<uint32_t> getShaderCode( const std::string & shaderName )
    {
      std::string sourcePath = "./shaders/" + shaderName;
      std::string compiledPath = "./compiled/" + shaderName + ".spv";

      if ( !std::filesystem::exists( sourcePath ) )
        throw std::runtime_error( "Shader source file does not exist: " + sourcePath );

      auto sourceTime = std::filesystem::last_write_time( sourcePath );

      bool compiledExists = std::filesystem::exists( compiledPath );
      bool needsRecompilation = !compiledExists;

      if ( compiledExists )
      {
        auto compiledTime = std::filesystem::last_write_time( compiledPath );
        needsRecompilation = sourceTime > compiledTime;
      }

      if ( needsRecompilation )
      {
        std::println( "Compiling shader from source: {} (source newer or compiled missing)", shaderName );
        return compileShader( shaderName );
      }

      std::vector<uint32_t> spirv = readCompiledShader( compiledPath );
      if ( !spirv.empty() )
      {
        std::println( "Read compiled shader from: {}", compiledPath );
        return spirv;
      }

      std::println( "Compiled shader file invalid, recompiling: {}", shaderName );
      return compileShader( shaderName );
    }

  }  // namespace help
}  // namespace core
