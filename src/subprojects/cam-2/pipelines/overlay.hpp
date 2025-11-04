#pragma once
#include "../data.hpp"
#include "../init.hpp"
#include "../state.hpp"
#include "bootstrap.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace pipelines
{
  namespace overlay
  {
    inline void recordCommandBuffer(
      vk::raii::CommandBuffer &          cmd,
      init::raii::ShaderBundle &         shaderBundle,
      core::SwapchainBundle &            swapchainBundle,
      uint32_t                           imageIndex,
      VkBuffer                           vertexBuffer,
      VkBuffer                           instanceBuffer,
      uint32_t                           instanceCount )
    {
      cmd.reset();
      cmd.begin( vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

      vk::ImageSubresourceRange subresourceRange{};
      subresourceRange.setAspectMask( vk::ImageAspectFlagBits::eColor ).setLevelCount( 1 ).setLayerCount( 1 );

      vk::ImageMemoryBarrier2 barrier{};
      barrier.setSrcStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        .setSrcAccessMask( vk::AccessFlagBits2::eNone )
        .setDstStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        .setDstAccessMask( vk::AccessFlagBits2::eColorAttachmentWrite )
        .setOldLayout( vk::ImageLayout::eUndefined )
        .setNewLayout( vk::ImageLayout::eColorAttachmentOptimal )
        .setSrcQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setDstQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setImage( swapchainBundle.images[imageIndex] )
        .setSubresourceRange( subresourceRange );

      vk::DependencyInfo depInfo{};
      depInfo.setImageMemoryBarrierCount( 1 ).setPImageMemoryBarriers( &barrier );

      // Transition depth image to depth attachment optimal layout
      vk::ImageSubresourceRange depthSubresourceRange{};
      depthSubresourceRange.setAspectMask( vk::ImageAspectFlagBits::eDepth ).setLevelCount( 1 ).setLayerCount( 1 );


      vk::DependencyInfo                     imageBarriers{};
      std::array<vk::ImageMemoryBarrier2, 2> barriers = {  barrier };
      imageBarriers.setImageMemoryBarrierCount( barriers.size() ).setPImageMemoryBarriers( barriers.data() );

      cmd.pipelineBarrier2( imageBarriers );

      vk::ClearValue clearValue{};
      clearValue.color = vk::ClearColorValue( std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f } );

      vk::ClearValue depthClearValue{};
      depthClearValue.depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

      vk::RenderingAttachmentInfo colorAttachment{};
      colorAttachment.setImageView( swapchainBundle.imageViews[imageIndex] )
        .setImageLayout( vk::ImageLayout::eColorAttachmentOptimal )
        .setLoadOp( vk::AttachmentLoadOp::eClear )
        .setStoreOp( vk::AttachmentStoreOp::eStore )
        .setClearValue( clearValue );



      vk::Rect2D renderArea{};
      renderArea.setExtent( swapchainBundle.extent );

      vk::RenderingInfo renderingInfo{};
      renderingInfo.setRenderArea( renderArea )
        .setLayerCount( 1 )
        .setColorAttachmentCount( 1 )
        .setPColorAttachments( &colorAttachment );

      cmd.beginRendering( renderingInfo );

      std::array<vk::ShaderStageFlagBits, 2> stages  = { vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment };
      std::array<vk::ShaderEXT, 2>           shaders = { *shaderBundle.getCurrentVertexShader(), *shaderBundle.getCurrentFragmentShader() };
      cmd.bindShadersEXT( stages, shaders );

      vk::Viewport viewport{ 0, 0, float( swapchainBundle.extent.width ), float( swapchainBundle.extent.height ), 0.0f, 1.0f };
      vk::Rect2D   scissor{ { 0, 0 }, swapchainBundle.extent };
      cmd.setViewportWithCount( viewport );
      cmd.setScissorWithCount( scissor );

      // Set up vertex input state - binding 0 for per-vertex data, binding 1 for per-instance data
      std::array<vk::VertexInputBindingDescription2EXT, 2> bindingDescs{};
      bindingDescs[0].setBinding( 0 ).setStride( sizeof( data::Vertex ) ).setInputRate( vk::VertexInputRate::eVertex ).setDivisor( 1 );
      bindingDescs[1].setBinding( 1 ).setStride( sizeof( data::InstanceData ) ).setInputRate( vk::VertexInputRate::eInstance ).setDivisor( 1 );

      std::array<vk::VertexInputAttributeDescription2EXT, 3> attributeDescs{};
      // Per-vertex attributes
      attributeDescs[0].setLocation( 0 ).setBinding( 0 ).setFormat( vk::Format::eR32G32Sfloat ).setOffset( offsetof( data::Vertex, position ) );
      attributeDescs[1].setLocation( 1 ).setBinding( 0 ).setFormat( vk::Format::eR32G32B32Sfloat ).setOffset( offsetof( data::Vertex, color ) );
      // Per-instance attribute
      attributeDescs[2].setLocation( 2 ).setBinding( 1 ).setFormat( vk::Format::eR32G32B32Sfloat ).setOffset( offsetof( data::InstanceData, position ) );

      cmd.setVertexInputEXT( bindingDescs, attributeDescs );

      // Bind vertex and instance buffers
      vk::DeviceSize offset = 0;
      cmd.bindVertexBuffers( 0, { vertexBuffer }, { offset } );
      cmd.bindVertexBuffers( 1, { instanceBuffer }, { offset } );

      cmd.setRasterizerDiscardEnable( VK_FALSE );
      cmd.setCullMode( vk::CullModeFlagBits::eNone );
      cmd.setFrontFace( vk::FrontFace::eCounterClockwise );
      cmd.setDepthTestEnable( VK_FALSE );
      cmd.setDepthWriteEnable( VK_FALSE );
      cmd.setDepthCompareOp( vk::CompareOp::eLess );
      cmd.setDepthBiasEnable( VK_FALSE );
      cmd.setStencilTestEnable( VK_FALSE );
      cmd.setPrimitiveTopology( vk::PrimitiveTopology::eTriangleStrip );
      cmd.setPrimitiveRestartEnable( VK_FALSE );
      cmd.setPolygonModeEXT( vk::PolygonMode::eFill );
      cmd.setRasterizationSamplesEXT( vk::SampleCountFlagBits::e1 );

      vk::SampleMask sampleMask = 0xFFFFFFFF;
      cmd.setSampleMaskEXT( vk::SampleCountFlagBits::e1, sampleMask );
      cmd.setAlphaToCoverageEnableEXT( VK_FALSE );
      cmd.setColorBlendEnableEXT( 0, VK_FALSE );
      cmd.setColorBlendEquationEXT( 0, vk::ColorBlendEquationEXT{} );
      vk::ColorComponentFlags colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
      cmd.setColorWriteMaskEXT( 0, colorWriteMask );


      cmd.draw( 3, instanceCount, 0, 0 );

      if ( !state::fpsMode )
      {
        // Render ImGui on top
        ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), *cmd );
      }

      cmd.endRendering();

      barrier.setSrcStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        .setSrcAccessMask( vk::AccessFlagBits2::eColorAttachmentWrite )
        .setDstStageMask( vk::PipelineStageFlagBits2::eBottomOfPipe )
        .setDstAccessMask( vk::AccessFlagBits2::eNone )
        .setOldLayout( vk::ImageLayout::eColorAttachmentOptimal )
        .setNewLayout( vk::ImageLayout::ePresentSrcKHR );

      depInfo.setImageMemoryBarrierCount( 1 ).setPImageMemoryBarriers( &barrier );

      cmd.pipelineBarrier2( depInfo );

      cmd.end();
    }
  }  // namespace basic
}  // namespace pipelines
