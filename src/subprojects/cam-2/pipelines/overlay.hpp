#pragma once
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
      init::raii::ColorTarget const &    srcColor,
      core::SwapchainBundle &            swapchainBundle,
      uint32_t                           imageIndex,
      bool                               renderImgui = true )
    {
      cmd.reset();
      cmd.begin( vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

      vk::ImageSubresourceRange subresourceRange{};
      subresourceRange.setAspectMask( vk::ImageAspectFlagBits::eColor ).setLevelCount( 1 ).setLayerCount( 1 );

      // Prepare swapchain as blit destination
      vk::ImageMemoryBarrier2 dstBarrier{};
      dstBarrier.setSrcStageMask( vk::PipelineStageFlagBits2::eTopOfPipe )
        .setSrcAccessMask( vk::AccessFlagBits2::eNone )
        .setDstStageMask( vk::PipelineStageFlagBits2::eTransfer )
        .setDstAccessMask( vk::AccessFlagBits2::eTransferWrite )
        .setOldLayout( vk::ImageLayout::eUndefined )
        .setNewLayout( vk::ImageLayout::eTransferDstOptimal )
        .setSrcQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setDstQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
        .setImage( swapchainBundle.images[imageIndex] )
        .setSubresourceRange( subresourceRange );

      // srcColor is already transitioned to eTransferSrcOptimal by basic pass
      vk::DependencyInfo depInfo{};
      depInfo.setImageMemoryBarrierCount( 1 ).setPImageMemoryBarriers( &dstBarrier );

      cmd.pipelineBarrier2( depInfo );

      // Blit from offscreen color to swapchain
      vk::ImageBlit2 blit{};
      blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
      blit.srcOffsets[1] = vk::Offset3D{ static_cast<int32_t>( srcColor.extent.width ), static_cast<int32_t>( srcColor.extent.height ), 1 };
      blit.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
      blit.dstOffsets[1] = vk::Offset3D{ static_cast<int32_t>( swapchainBundle.extent.width ), static_cast<int32_t>( swapchainBundle.extent.height ), 1 };
      blit.srcSubresource.setAspectMask( vk::ImageAspectFlagBits::eColor ).setMipLevel( 0 ).setBaseArrayLayer( 0 ).setLayerCount( 1 );
      blit.dstSubresource.setAspectMask( vk::ImageAspectFlagBits::eColor ).setMipLevel( 0 ).setBaseArrayLayer( 0 ).setLayerCount( 1 );

      vk::BlitImageInfo2 blitInfo{};
      blitInfo.setSrcImage( srcColor.image )
        .setSrcImageLayout( vk::ImageLayout::eTransferSrcOptimal )
        .setDstImage( swapchainBundle.images[imageIndex] )
        .setDstImageLayout( vk::ImageLayout::eTransferDstOptimal )
        .setRegions( blit );
      cmd.blitImage2( blitInfo );

      // Transition swapchain to color attachment for ImGui
      dstBarrier.setSrcStageMask( vk::PipelineStageFlagBits2::eTransfer )
        .setSrcAccessMask( vk::AccessFlagBits2::eTransferWrite )
        .setDstStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        .setDstAccessMask( vk::AccessFlagBits2::eColorAttachmentWrite )
        .setOldLayout( vk::ImageLayout::eTransferDstOptimal )
        .setNewLayout( vk::ImageLayout::eColorAttachmentOptimal );
      depInfo.setImageMemoryBarrierCount( 1 ).setPImageMemoryBarriers( &dstBarrier );
      cmd.pipelineBarrier2( depInfo );

      vk::RenderingAttachmentInfo colorAttachment{};
      colorAttachment.setImageView( swapchainBundle.imageViews[imageIndex] )
        .setImageLayout( vk::ImageLayout::eColorAttachmentOptimal )
        .setLoadOp( vk::AttachmentLoadOp::eLoad )
        .setStoreOp( vk::AttachmentStoreOp::eStore )
        .setClearValue( {} );



      vk::Rect2D renderArea{};
      renderArea.setExtent( swapchainBundle.extent );

      vk::RenderingInfo renderingInfo{};
      renderingInfo.setRenderArea( renderArea )
        .setLayerCount( 1 )
        .setColorAttachmentCount( 1 )
        .setPColorAttachments( &colorAttachment );

      cmd.beginRendering( renderingInfo );

      if ( renderImgui && !state::fpvMode )
      {
        // Render ImGui on top
        ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), *cmd );
      }

      cmd.endRendering();

      dstBarrier.setSrcStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        .setSrcAccessMask( vk::AccessFlagBits2::eColorAttachmentWrite )
        .setDstStageMask( vk::PipelineStageFlagBits2::eBottomOfPipe )
        .setDstAccessMask( vk::AccessFlagBits2::eNone )
        .setOldLayout( vk::ImageLayout::eColorAttachmentOptimal )
        .setNewLayout( vk::ImageLayout::ePresentSrcKHR );

      depInfo.setImageMemoryBarrierCount( 1 ).setPImageMemoryBarriers( &dstBarrier );

      cmd.pipelineBarrier2( depInfo );

      cmd.end();
    }
  }  // namespace overlay
}  // namespace pipelines
