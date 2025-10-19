#include "helper.hpp"

#include "converter.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <print>
#include <stdexcept>

namespace core
{
  namespace help
  {

    std::vector<uint32_t> readCompiledShader( const std::string & compiledPath )
    {
      std::ifstream compiledFile( compiledPath, std::ios::binary | std::ios::ate );

      if ( compiledFile.is_open() )
      {
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
        compiledFile.close();
      }
      return {};
    }

    std::string readShaderSource( const std::string & shaderPath )
    {
      std::ifstream sourceFile( shaderPath );
      if ( !sourceFile.is_open() )
      {
        throw std::runtime_error( "Failed to open shader source file: " + shaderPath );
      }
      std::stringstream buffer;
      buffer << sourceFile.rdbuf();
      std::string source = buffer.str();
      sourceFile.close();
      return source;
    }

    std::vector<uint32_t> compileShader( const std::string & shaderName )
    {
      // Determine shader stage from extension
      shaderc_shader_kind kind = core::to::shadercKind( core::help::vkStageFromShaderName( shaderName ) );

      std::string sourcePath = "./shaders/" + shaderName;
      // Read source file
      std::string source = readShaderSource( sourcePath );

      // Compile the shader
      shaderc::Compiler       compiler;
      shaderc::CompileOptions options;
      options.SetOptimizationLevel( shaderc_optimization_level_performance );

      shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv( source, kind, shaderName.c_str(), options );

      if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
      {
        throw std::runtime_error( "Shader compilation failed for '" + sourcePath + "': " + result.GetErrorMessage() );
      }

      // Convert result to vector
      std::vector<uint32_t> spirv( result.cbegin(), result.cend() );

      // Extract filename without extension for output
      std::string outputPath = "./compiled/" + shaderName + ".spv";

      // Create compiled directory if it doesn't exist
      std::filesystem::create_directories( "./compiled" );

      // Write compiled shader
      std::ofstream outFile( outputPath, std::ios::binary );
      if ( !outFile.is_open() )
      {
        throw std::runtime_error( "Failed to create output file: " + outputPath );
      }

      outFile.write( reinterpret_cast<const char *>( spirv.data() ), spirv.size() * sizeof( uint32_t ) );
      outFile.close();

      return spirv;
    }

    std::vector<uint32_t> getShaderCode( const std::string & shaderName )
    {
      // First, try to read from /compiled directory
      std::string compiledPath = "./compiled/" + shaderName + ".spv";

      std::vector<uint32_t> spirv = readCompiledShader( compiledPath );
      if ( !spirv.empty() )
      {
        std::println( "Read compiled shader from: {}", compiledPath );
        return spirv;
      }

      std::println( "Compiling shader from source: {}", shaderName );
      // If compiled file doesn't exist or is invalid, compile from source
      return compileShader( shaderName );
    }



  }  // namespace help
}  // namespace core
