#include "shaderobj.hpp"
#include "shaderc/env.h"
#include <spirv_reflect.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstring>

namespace core
{

  static std::vector<uint32_t> obtainShaderSpirV(const std::string& shaderName)
  {
    // Look for compiled SPIR-V in "/compiled/[shaderName].spv"
    std::string spvPath = "./compiled/" + shaderName + ".spv";
    std::ifstream spvFile(spvPath, std::ios::binary | std::ios::ate);
    if (spvFile.is_open()) {
      std::streamsize size = spvFile.tellg();
      spvFile.seekg(0, std::ios::beg);
      if (size % 4 != 0 || size <= 0) {
        // Invalid SPIR-V file size, treat as missing and fall through to compilation
      } else {
        std::vector<uint32_t> spirv(static_cast<size_t>(size) / 4);
        if (spvFile.read(reinterpret_cast<char*>(spirv.data()), size)) {
          return spirv;
        }
        // If file couldn't be properly read, fall through to compilation
      }
      // If file open but error, fall through
    }

    // Compiled file didn't exist or failed to read, so fall back to source in "/shaders/[shaderName]"
    std::string srcPath = "./shaders/" + shaderName;
    std::ifstream srcFile(srcPath);
    if (!srcFile.is_open()) {
      throw std::runtime_error("Failed to open shader source file: " + srcPath);
    }
    std::stringstream buffer;
    buffer << srcFile.rdbuf();
    std::string source = buffer.str();

    // For this utility function, make some assumptions: 
    //   * Use default compile options & unknown stage
    //   * You may want to customize
    ShaderCompileOptions defaultOptions;
    ShaderCompileResult compileResult = compileShaderFromSource(source, ShaderStage::Unknown, defaultOptions, shaderName);

    if (!compileResult.success) {
      throw std::runtime_error("Shader compilation failed for '" + shaderName + "': " + compileResult.errorMessage);
    }

    return compileResult.spirv;
  }


  ShaderCompileResult compileShaderFromSource(
    const std::string &          source,
    ShaderStage                  stage,
    const ShaderCompileOptions & options,
    const std::string &          sourceName )
  {
    ShaderCompileResult result;

    // Create compiler and options
    shaderc::Compiler       compiler;
    shaderc::CompileOptions compileOptions;

    // Set optimization level
    if ( options.optimize )
    {
      compileOptions.SetOptimizationLevel( options.optimizationLevel );
    }

    // Set debug info generation
    if ( options.generateDebugInfo )
    {
      compileOptions.SetGenerateDebugInfo();
    }

    // Add macro definitions
    for ( const auto & macro : options.macroDefinitions )
    {
      size_t equalsPos = macro.find( '=' );
      if ( equalsPos != std::string::npos )
      {
        // Macro with value: NAME=VALUE
        std::string name  = macro.substr( 0, equalsPos );
        std::string value = macro.substr( equalsPos + 1 );
        compileOptions.AddMacroDefinition( name, value );
      }
      else
      {
        // Macro without value: NAME
        compileOptions.AddMacroDefinition( macro );
      }
    }

    // Set target environment
    compileOptions.SetTargetEnvironment( shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4 );
    compileOptions.SetTargetSpirv( shaderc_spirv_version_1_6 );

    // Compile the shader
    shaderc_shader_kind        kind             = shaderStageToShadercKind( stage );
    shaderc::SpvCompilationResult compilationResult = compiler.CompileGlslToSpv(
      source,
      kind,
      sourceName.c_str(),
      options.entryPoint.c_str(),
      compileOptions );

    // Check compilation status
    result.status      = compilationResult.GetCompilationStatus();
    result.numWarnings = compilationResult.GetNumWarnings();
    result.numErrors   = compilationResult.GetNumErrors();

    if ( result.status != shaderc_compilation_status_success )
    {
      result.success      = false;
      result.errorMessage = compilationResult.GetErrorMessage();
      return result;
    }

    // Extract SPIR-V code
    result.success = true;
    result.spirv.assign( compilationResult.begin(), compilationResult.end() );

    // Get any warnings
    if ( result.numWarnings > 0 )
    {
      result.errorMessage = compilationResult.GetErrorMessage();
    }

    return result;
  }


  ShaderCompileResult compileShaderFromFile(
    const std::string &          filePath,
    ShaderStage                  stage,
    const ShaderCompileOptions & options )
  {
    // Read the shader file
    std::ifstream file( filePath );
    if ( !file.is_open() )
    {
      ShaderCompileResult result;
      result.success      = false;
      result.errorMessage = "Failed to open shader file: " + filePath;
      result.status       = shaderc_compilation_status_compilation_error;
      return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Compile using the source compilation function
    return compileShaderFromSource( source, stage, options, filePath );
  }



  // Reflect SPIR-V and extract descriptor sets, push constants, etc.
  static ShaderReflectionData reflectSpirv( const std::vector<uint32_t> & spirv )
  {
    ShaderReflectionData reflectionData;

    // Create reflection module
    SpvReflectShaderModule module{};
    SpvReflectResult result = spvReflectCreateShaderModule(
      spirv.size() * sizeof( uint32_t ),
      spirv.data(),
      &module );

    if ( result != SPV_REFLECT_RESULT_SUCCESS )
    {
      throw std::runtime_error( "Failed to create SPIR-V reflection module" );
    }

    // Get shader stage
    reflectionData.shaderStage = convertReflectShaderStage( static_cast<SpvReflectShaderStageFlagBits>( module.shader_stage ) );
    reflectionData.entryPoint  = module.entry_point_name ? module.entry_point_name : "main";

    // Get descriptor sets
    uint32_t descriptorSetCount = 0;
    result = spvReflectEnumerateDescriptorSets( &module, &descriptorSetCount, nullptr );
    if ( result == SPV_REFLECT_RESULT_SUCCESS && descriptorSetCount > 0 )
    {
      std::vector<SpvReflectDescriptorSet *> descriptorSets( descriptorSetCount );
      result = spvReflectEnumerateDescriptorSets( &module, &descriptorSetCount, descriptorSets.data() );

      if ( result == SPV_REFLECT_RESULT_SUCCESS )
      {
        reflectionData.dataFlags.hasDescriptorSets = true;
        reflectionData.descriptorSets.reserve( descriptorSetCount );

        for ( const auto * set : descriptorSets )
        {
          ShaderReflectionData::DescriptorSetLayoutData layoutData;
          layoutData.setNumber = set->set;
          layoutData.bindings.reserve( set->binding_count );

          for ( uint32_t i = 0; i < set->binding_count; ++i )
          {
            const auto * binding = set->bindings[i];

            vk::DescriptorSetLayoutBinding vkBinding;
            vkBinding.binding         = binding->binding;
            vkBinding.descriptorType  = convertReflectDescriptorType( binding->descriptor_type );
            vkBinding.descriptorCount = binding->count;
            vkBinding.stageFlags      = reflectionData.shaderStage;
            vkBinding.pImmutableSamplers = nullptr;

            layoutData.bindings.push_back( vkBinding );
          }

          reflectionData.descriptorSets.push_back( layoutData );
        }

        // Sort descriptor sets by set number
        std::sort( reflectionData.descriptorSets.begin(), reflectionData.descriptorSets.end(),
                   []( const auto & a, const auto & b ) { return a.setNumber < b.setNumber; } );
      }
    }

    // Get push constants
    uint32_t pushConstantCount = 0;
    result = spvReflectEnumeratePushConstantBlocks( &module, &pushConstantCount, nullptr );
    if ( result == SPV_REFLECT_RESULT_SUCCESS && pushConstantCount > 0 )
    {
      std::vector<SpvReflectBlockVariable *> pushConstants( pushConstantCount );
      result = spvReflectEnumeratePushConstantBlocks( &module, &pushConstantCount, pushConstants.data() );

      if ( result == SPV_REFLECT_RESULT_SUCCESS )
      {
        reflectionData.dataFlags.hasPushConstants = true;
        reflectionData.pushConstants.reserve( pushConstantCount );

        for ( const auto * block : pushConstants )
        {
          vk::PushConstantRange range;
          range.stageFlags = reflectionData.shaderStage;
          range.offset     = block->offset;
          range.size       = block->size;

          reflectionData.pushConstants.push_back( range );
        }
      }
    }

    // Get vertex input attributes (for vertex shaders)
    if ( reflectionData.shaderStage == vk::ShaderStageFlagBits::eVertex )
    {
      uint32_t inputVarCount = 0;
      result = spvReflectEnumerateInputVariables( &module, &inputVarCount, nullptr );
      if ( result == SPV_REFLECT_RESULT_SUCCESS && inputVarCount > 0 )
      {
        std::vector<SpvReflectInterfaceVariable *> inputVars( inputVarCount );
        result = spvReflectEnumerateInputVariables( &module, &inputVarCount, inputVars.data() );

        if ( result == SPV_REFLECT_RESULT_SUCCESS )
        {
          for ( const auto * var : inputVars )
          {
            // Skip built-in variables
            if ( var->built_in != -1 )
            {
              continue;
            }

            vk::VertexInputAttributeDescription attr;
            attr.location = var->location;
            attr.binding  = 0;  // User needs to set this
            attr.format   = convertReflectFormat( var->format );
            attr.offset   = 0;  // User needs to calculate this

            reflectionData.vertexInputAttributes.push_back( attr );
          }

          if ( !reflectionData.vertexInputAttributes.empty() )
          {
            reflectionData.dataFlags.hasVertexInputs = true;
          }
        }
      }
    }

    // Clean up reflection module
    spvReflectDestroyShaderModule( &module );

    return reflectionData;
  }

  ShaderObjectResult createShaderObject(
    VkDevice                     device,
    const std::string &          source,
    ShaderStage                  stage,
    const ShaderCompileOptions & options,
    const std::string &          sourceName )
  {
    ShaderObjectResult result;

    // Step 1: Compile shader source to SPIR-V
    ShaderCompileResult compileResult = compileShaderFromSource( source, stage, options, sourceName );
    if ( !compileResult.success )
    {
      result.success      = false;
      result.errorMessage = "Shader compilation failed: " + compileResult.errorMessage;
      return result;
    }

    result.spirv = compileResult.spirv;

    // Step 2: Reflect SPIR-V to extract descriptor sets, push constants, etc.
    try
    {
      result.reflectionData = reflectSpirv( compileResult.spirv );
    }
    catch ( const std::exception & e )
    {
      result.success      = false;
      result.errorMessage = "SPIR-V reflection failed: " + std::string( e.what() );
      return result;
    }

    // Step 3: Create VkShaderEXT using VK_EXT_shader_object
    VkShaderCreateInfoEXT shaderCreateInfo{};
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
    shaderCreateInfo.pNext = nullptr;
    shaderCreateInfo.flags = 0;
    shaderCreateInfo.stage = shaderStageToVkShaderStage( stage );
    shaderCreateInfo.nextStage = 0;  // Will be set based on pipeline stage
    shaderCreateInfo.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
    shaderCreateInfo.codeSize = compileResult.spirv.size() * sizeof( uint32_t );
    shaderCreateInfo.pCode = compileResult.spirv.data();
    shaderCreateInfo.pName = result.reflectionData.entryPoint.c_str();
    shaderCreateInfo.setLayoutCount = 0;
    shaderCreateInfo.pSetLayouts = nullptr;
    shaderCreateInfo.pushConstantRangeCount = static_cast<uint32_t>( result.reflectionData.pushConstants.size() );
    shaderCreateInfo.pPushConstantRanges = result.reflectionData.pushConstants.empty() ? nullptr : reinterpret_cast<const VkPushConstantRange *>( result.reflectionData.pushConstants.data() );
    shaderCreateInfo.pSpecializationInfo = nullptr;

    // Get the function pointer for vkCreateShadersEXT
    auto vkCreateShadersEXT = reinterpret_cast<PFN_vkCreateShadersEXT>(
      vkGetDeviceProcAddr( device, "vkCreateShadersEXT" ) );

    if ( !vkCreateShadersEXT )
    {
      result.success      = false;
      result.errorMessage = "VK_EXT_shader_object extension not available (vkCreateShadersEXT not found)";
      return result;
    }

    // Create the shader object
    VkResult vkResult = vkCreateShadersEXT( device, 1, &shaderCreateInfo, nullptr, &result.shaderEXT );
    if ( vkResult != VK_SUCCESS )
    {
      result.success      = false;
      result.errorMessage = "Failed to create shader object (VkResult: " + std::to_string( vkResult ) + ")";
      return result;
    }

    result.success = true;
    return result;
  }

  ShaderObjectResult createShaderObjectFromFile(
    VkDevice                     device,
    const std::string &          filePath,
    ShaderStage                  stage,
    const ShaderCompileOptions & options )
  {
    // Read the shader file
    std::ifstream file( filePath );
    if ( !file.is_open() )
    {
      ShaderObjectResult result;
      result.success      = false;
      result.errorMessage = "Failed to open shader file: " + filePath;
      return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Create shader object using the source function
    return createShaderObject( device, source, stage, options, filePath );
  }

  void destroyShaderObject( VkDevice device, VkShaderEXT shaderEXT )
  {
    if ( shaderEXT == VK_NULL_HANDLE )
    {
      return;
    }

    // Get the function pointer for vkDestroyShaderEXT
    auto vkDestroyShaderEXT = reinterpret_cast<PFN_vkDestroyShaderEXT>(
      vkGetDeviceProcAddr( device, "vkDestroyShaderEXT" ) );

    if ( vkDestroyShaderEXT )
    {
      vkDestroyShaderEXT( device, shaderEXT, nullptr );
    }
  }

}