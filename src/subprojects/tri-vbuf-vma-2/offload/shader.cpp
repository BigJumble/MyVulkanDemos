#include "shader.hpp"

#include "helper.hpp"
#include "types.hpp"

namespace offload
{

  vk::raii::PipelineLayout createPipelineLayout( vk::raii::Device const & device )
  {
    vk::PushConstantRange pushConstantRange{};
    pushConstantRange.setStageFlags( vk::ShaderStageFlagBits::eVertex ).setSize( sizeof( PushConstants ) ).setOffset( 0 );

    vk::PipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.setPushConstantRangeCount( 1 ).setPPushConstantRanges( &pushConstantRange );

    return vk::raii::PipelineLayout{ device, layoutInfo };
  }

  ShaderObjects createShaderObjects( vk::raii::Device const & device, const vk::PushConstantRange & pushConstantRange )
  {
    std::vector<uint32_t> vertShaderCode = core::help::getShaderCode( "triangle.vert" );
    std::vector<uint32_t> fragShaderCode = core::help::getShaderCode( "triangle.frag" );

    vk::ShaderCreateInfoEXT vertInfo{};
    vertInfo.setStage( vk::ShaderStageFlagBits::eVertex )
      .setCodeType( vk::ShaderCodeTypeEXT::eSpirv )
      .setNextStage( vk::ShaderStageFlagBits::eFragment )
      .setPCode( vertShaderCode.data() )
      .setPName( "main" )
      .setCodeSize( vertShaderCode.size() * sizeof( uint32_t ) )
      .setPushConstantRangeCount( 1 )
      .setPPushConstantRanges( &pushConstantRange );

    vk::ShaderCreateInfoEXT fragInfo{};
    fragInfo.setStage( vk::ShaderStageFlagBits::eFragment )
      .setCodeType( vk::ShaderCodeTypeEXT::eSpirv )
      .setNextStage( {} )
      .setPCode( fragShaderCode.data() )
      .setPName( "main" )
      .setCodeSize( fragShaderCode.size() * sizeof( uint32_t ) )
      .setPushConstantRangeCount( 1 )
      .setPPushConstantRanges( &pushConstantRange );

    return ShaderObjects{ vk::raii::ShaderEXT{ device, vertInfo }, vk::raii::ShaderEXT{ device, fragInfo } };
  }

}  // namespace offload
