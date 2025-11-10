#pragma once
#include "../data.hpp"
#include "../init.hpp"
#include "../state.hpp"
#include "bootstrap.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace pipelines
{
  namespace basic
  {
    inline void recordCommandBufferOffscreen(
      vk::raii::CommandBuffer &          cmd,
      init::raii::ShaderBundle &         shaderBundle,
      init::raii::ColorTarget const &    colorTarget,
      VkBuffer                           vertexBuffer,
      VkBuffer                           instanceBuffer,
      uint32_t                           instanceCount,
      init::raii::DepthResources const & depthResources )
    {
      cmd.reset();
      cmd.begin( vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

      vk::ImageSubresourceRange subresourceRange{};
      subresourceRange.setAspectMask( vk::ImageAspectFlagBits::eColor ).setLevelCount( 1 ).setLayerCount( 1 );

      vk::ImageMemoryBarrier2 colorBarrier{};
      colorBarrier.setSrcStageMask( vk::PipelineStageFlagBits2::eTopOfPipe )
        .setSrcAccessMask( vk::AccessFlagBits2::eNone )
        .setDstStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        .setDstAccessMask( vk::AccessFlagBits2::eColorAttachmentWrite )
        .setOldLayout( vk::ImageLayout::eUndefined )
        .setNewLayout( vk::ImageLayout::eColorAttachmentOptimal )
        .setSrcQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setDstQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setImage( colorTarget.image )
        .setSubresourceRange( subresourceRange );

      // Transition depth (keep in optimal layout)
      vk::ImageSubresourceRange depthSubresourceRange{};
      depthSubresourceRange.setAspectMask( vk::ImageAspectFlagBits::eDepth ).setLevelCount( 1 ).setLayerCount( 1 );

      vk::ImageMemoryBarrier2 depthBarrier{};
      depthBarrier.setSrcStageMask( vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests )
        .setSrcAccessMask( vk::AccessFlagBits2::eDepthStencilAttachmentWrite )
        .setDstStageMask( vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests )
        .setDstAccessMask( vk::AccessFlagBits2::eDepthStencilAttachmentWrite )
        .setOldLayout( vk::ImageLayout::eDepthAttachmentOptimal )
        .setNewLayout( vk::ImageLayout::eDepthAttachmentOptimal )
        .setSrcQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setDstQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setImage( depthResources.image )
        .setSubresourceRange( depthSubresourceRange );

      std::array<vk::ImageMemoryBarrier2, 2> barriers = { depthBarrier, colorBarrier };
      vk::DependencyInfo                      depInfo{};
      depInfo.setImageMemoryBarrierCount( barriers.size() ).setPImageMemoryBarriers( barriers.data() );
      cmd.pipelineBarrier2( depInfo );

      vk::ClearValue clearValue{};
      clearValue.color = vk::ClearColorValue( std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f } );

      vk::ClearValue depthClearValue{};
      depthClearValue.depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

      vk::RenderingAttachmentInfo colorAttachment{};
      colorAttachment.setImageView( colorTarget.imageView )
        .setImageLayout( vk::ImageLayout::eColorAttachmentOptimal )
        .setLoadOp( vk::AttachmentLoadOp::eClear )
        .setStoreOp( vk::AttachmentStoreOp::eStore )
        .setClearValue( clearValue );

      vk::RenderingAttachmentInfo depthAttachment{};
      depthAttachment.setImageView( depthResources.imageView )
        .setImageLayout( vk::ImageLayout::eDepthAttachmentOptimal )
        .setLoadOp( vk::AttachmentLoadOp::eClear )
        .setStoreOp( vk::AttachmentStoreOp::eDontCare )
        .setClearValue( depthClearValue );

      vk::Rect2D renderArea{};
      renderArea.setExtent( colorTarget.extent );

      vk::RenderingInfo renderingInfo{};
      renderingInfo.setRenderArea( renderArea )
        .setLayerCount( 1 )
        .setColorAttachmentCount( 1 )
        .setPColorAttachments( &colorAttachment )
        .setPDepthAttachment( &depthAttachment );

      cmd.beginRendering( renderingInfo );

      std::array<vk::ShaderStageFlagBits, 2> stages  = { vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment };
      std::array<vk::ShaderEXT, 2>           shaders = { *shaderBundle.getCurrentVertexShader(), *shaderBundle.getCurrentFragmentShader() };
      cmd.bindShadersEXT( stages, shaders );

      vk::Viewport viewport{ 0, 0, float( colorTarget.extent.width ), float( colorTarget.extent.height ), 0.0f, 1.0f };
      vk::Rect2D   scissor{ { 0, 0 }, colorTarget.extent };
      cmd.setViewportWithCount( viewport );
      cmd.setScissorWithCount( scissor );

      std::array<vk::VertexInputBindingDescription2EXT, 2> bindingDescs{};
      bindingDescs[0].setBinding( 0 ).setStride( sizeof( data::Vertex ) ).setInputRate( vk::VertexInputRate::eVertex ).setDivisor( 1 );
      bindingDescs[1].setBinding( 1 ).setStride( sizeof( data::InstanceData ) ).setInputRate( vk::VertexInputRate::eInstance ).setDivisor( 1 );

      std::array<vk::VertexInputAttributeDescription2EXT, 3> attributeDescs{};
      attributeDescs[0].setLocation( 0 ).setBinding( 0 ).setFormat( vk::Format::eR32G32Sfloat ).setOffset( offsetof( data::Vertex, position ) );
      attributeDescs[1].setLocation( 1 ).setBinding( 0 ).setFormat( vk::Format::eR32G32B32Sfloat ).setOffset( offsetof( data::Vertex, color ) );
      attributeDescs[2].setLocation( 2 ).setBinding( 1 ).setFormat( vk::Format::eR32G32B32Sfloat ).setOffset( offsetof( data::InstanceData, position ) );
      cmd.setVertexInputEXT( bindingDescs, attributeDescs );

      vk::DeviceSize offset = 0;
      cmd.bindVertexBuffers( 0, { vertexBuffer }, { offset } );
      cmd.bindVertexBuffers( 1, { instanceBuffer }, { offset } );

      cmd.setRasterizerDiscardEnable( state::rasterizerDiscardEnable ? VK_TRUE : VK_FALSE );
      cmd.setCullMode( state::cullMode );
      cmd.setFrontFace( state::frontFace );
      cmd.setDepthTestEnable( state::depthTestEnable ? VK_TRUE : VK_FALSE );
      cmd.setDepthWriteEnable( state::depthWriteEnable ? VK_TRUE : VK_FALSE );
      cmd.setDepthCompareOp( state::depthCompareOp );
      cmd.setDepthBiasEnable( state::depthBiasEnable ? VK_TRUE : VK_FALSE );
      cmd.setStencilTestEnable( state::stencilTestEnable ? VK_TRUE : VK_FALSE );
      cmd.setPrimitiveTopology( state::primitiveTopology );
      cmd.setPrimitiveRestartEnable( state::primitiveRestartEnable ? VK_TRUE : VK_FALSE );
      cmd.setPolygonModeEXT( state::polygonMode );
      if ( state::polygonMode == vk::PolygonMode::eLine )
      {
        cmd.setLineWidth( state::lineWidth );
      }
      cmd.setRasterizationSamplesEXT( vk::SampleCountFlagBits::e1 );

      vk::SampleMask sampleMask = 0xFFFFFFFF;
      cmd.setSampleMaskEXT( vk::SampleCountFlagBits::e1, sampleMask );
      cmd.setAlphaToCoverageEnableEXT( VK_FALSE );
      cmd.setColorBlendEnableEXT( 0, VK_FALSE );
      cmd.setColorBlendEquationEXT( 0, vk::ColorBlendEquationEXT{} );
      vk::ColorComponentFlags colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
      cmd.setColorWriteMaskEXT( 0, colorWriteMask );

      float     t = static_cast<float>( glfwGetTime() );
      glm::vec3 cameraPos    = glm::vec3( std::sin( t ) * 3.0f, 2.0f, std::cos( t ) * 3.0f );
      glm::vec3 cameraTarget = glm::vec3( 0.0f, 0.0f, 0.0f );
      glm::vec3 cameraUp     = glm::vec3( 0.0f, 1.0f, 0.0f );
      glm::mat4 view         = glm::lookAt( cameraPos, cameraTarget, cameraUp );

      float     aspect = float( colorTarget.extent.width ) / float( colorTarget.extent.height );
      glm::mat4 proj   = glm::perspective( glm::radians( 45.0f ), aspect, 0.1f, 10000.0f );
      proj[1][1] *= -1;

      data::PushConstants pc{ view, proj };
      cmd.pushConstants<data::PushConstants>( *shaderBundle.pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, { pc } );

      cmd.draw( 3, instanceCount, 0, 0 );

      cmd.endRendering();

      // Transition color target for blit (src)
      colorBarrier.setSrcStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        .setSrcAccessMask( vk::AccessFlagBits2::eColorAttachmentWrite )
        .setDstStageMask( vk::PipelineStageFlagBits2::eTransfer )
        .setDstAccessMask( vk::AccessFlagBits2::eTransferRead )
        .setOldLayout( vk::ImageLayout::eColorAttachmentOptimal )
        .setNewLayout( vk::ImageLayout::eTransferSrcOptimal );
      vk::DependencyInfo depInfo2{};
      depInfo2.setImageMemoryBarrierCount( 1 ).setPImageMemoryBarriers( &colorBarrier );
      cmd.pipelineBarrier2( depInfo2 );

      cmd.end();
    }

    inline void recordCommandBuffer(
      vk::raii::CommandBuffer &          cmd,
      init::raii::ShaderBundle &         shaderBundle,
      core::SwapchainBundle &            swapchainBundle,
      uint32_t                           imageIndex,
      VkBuffer                           vertexBuffer,
      VkBuffer                           instanceBuffer,
      uint32_t                           instanceCount,
      init::raii::DepthResources const & depthResources )
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

      vk::ImageMemoryBarrier2 depthBarrier{};
      depthBarrier.setSrcStageMask( vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests )
        .setSrcAccessMask( vk::AccessFlagBits2::eDepthStencilAttachmentWrite )
        .setDstStageMask( vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests )
        .setDstAccessMask( vk::AccessFlagBits2::eDepthStencilAttachmentWrite )
        .setOldLayout( vk::ImageLayout::eDepthAttachmentOptimal )
        .setNewLayout( vk::ImageLayout::eDepthAttachmentOptimal )
        .setSrcQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setDstQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setImage( depthResources.image )
        .setSubresourceRange( depthSubresourceRange );

      vk::DependencyInfo                     imageBarriers{};
      std::array<vk::ImageMemoryBarrier2, 2> barriers = { depthBarrier, barrier };
      imageBarriers.setImageMemoryBarrierCount( 2 ).setPImageMemoryBarriers( barriers.data() );

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

      vk::RenderingAttachmentInfo depthAttachment{};
      depthAttachment.setImageView( depthResources.imageView )
        .setImageLayout( vk::ImageLayout::eDepthAttachmentOptimal )
        .setLoadOp( vk::AttachmentLoadOp::eClear )
        .setStoreOp( vk::AttachmentStoreOp::eDontCare )
        .setClearValue( depthClearValue );

      vk::Rect2D renderArea{};
      renderArea.setExtent( swapchainBundle.extent );

      vk::RenderingInfo renderingInfo{};
      renderingInfo.setRenderArea( renderArea )
        .setLayerCount( 1 )
        .setColorAttachmentCount( 1 )
        .setPColorAttachments( &colorAttachment )
        .setPDepthAttachment( &depthAttachment );

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

      cmd.setRasterizerDiscardEnable( state::rasterizerDiscardEnable ? VK_TRUE : VK_FALSE );
      cmd.setCullMode( state::cullMode );
      cmd.setFrontFace( state::frontFace );
      cmd.setDepthTestEnable( state::depthTestEnable ? VK_TRUE : VK_FALSE );
      cmd.setDepthWriteEnable( state::depthWriteEnable ? VK_TRUE : VK_FALSE );
      cmd.setDepthCompareOp( state::depthCompareOp );
      cmd.setDepthBiasEnable( state::depthBiasEnable ? VK_TRUE : VK_FALSE );
      cmd.setStencilTestEnable( state::stencilTestEnable ? VK_TRUE : VK_FALSE );
      cmd.setPrimitiveTopology( state::primitiveTopology );
      cmd.setPrimitiveRestartEnable( state::primitiveRestartEnable ? VK_TRUE : VK_FALSE );
      cmd.setPolygonModeEXT( state::polygonMode );
      if ( state::polygonMode == vk::PolygonMode::eLine )
      {
        cmd.setLineWidth( state::lineWidth );
      }
      cmd.setRasterizationSamplesEXT( vk::SampleCountFlagBits::e1 );

      vk::SampleMask sampleMask = 0xFFFFFFFF;
      cmd.setSampleMaskEXT( vk::SampleCountFlagBits::e1, sampleMask );
      cmd.setAlphaToCoverageEnableEXT( VK_FALSE );
      cmd.setColorBlendEnableEXT( 0, VK_FALSE );
      cmd.setColorBlendEquationEXT( 0, vk::ColorBlendEquationEXT{} );
      vk::ColorComponentFlags colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
      cmd.setColorWriteMaskEXT( 0, colorWriteMask );

      // Set push constants with camera view and projection matrices
      float t = static_cast<float>( glfwGetTime() );

      // Create a simple rotating camera
      glm::vec3 cameraPos    = glm::vec3( std::sin( t ) * 3.0f, 2.0f, std::cos( t ) * 3.0f );
      glm::vec3 cameraTarget = glm::vec3( 0.0f, 0.0f, 0.0f );
      glm::vec3 cameraUp     = glm::vec3( 0.0f, 1.0f, 0.0f );
      glm::mat4 view         = glm::lookAt( cameraPos, cameraTarget, cameraUp );

      // Create perspective projection
      float     aspect = float( swapchainBundle.extent.width ) / float( swapchainBundle.extent.height );
      glm::mat4 proj   = glm::perspective( glm::radians( 45.0f ), aspect, 0.1f, 10000.0f );
      proj[1][1] *= -1;  // Flip Y for Vulkan

      data::PushConstants pc{ view, proj };
      cmd.pushConstants<data::PushConstants>( *shaderBundle.pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, { pc } );

      cmd.draw( 3, instanceCount, 0, 0 );

      if ( !state::imguiMode )
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
