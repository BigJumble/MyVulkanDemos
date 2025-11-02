#pragma once
#include "imgui.h"
#include "state.hpp"

#include <string>
#include <vector>

namespace ui
{
  inline void renderStatsWindow()
  {
    ImGui::Begin( "Stats" );
    ImGui::Text( "FPS: %.1f", ImGui::GetIO().Framerate );
    ImGui::Text( "Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate );
    ImGui::End();

    // ImGui::ShowDemoWindow();
  }

  inline void renderPresentModeWindow()
  {
    ImGui::Begin( "Present Mode" );
    ImGui::Text( "Available Present Modes: %d", static_cast<int>( state::availablePresentModes.size() ) );
    // Make present modes selectable as radio buttons
    for ( size_t i = 0; i < state::availablePresentModes.size(); ++i )
    {
      std::string modeStr  = vk::to_string( state::availablePresentModes[i] );
      bool        selected = ( state::presentMode == state::availablePresentModes[i] );
      if ( ImGui::RadioButton( modeStr.c_str(), selected ) )
      {
        state::presentMode = state::availablePresentModes[i];
      }
    }
    ImGui::End();
  }

  // UI for pipeline rasterization, depth/stencil, primitive, multisample states
  inline void renderPipelineStateWindow()
  {
    ImGui::Begin( "Pipeline States" );

    // Rasterization
    if ( ImGui::CollapsingHeader( "Rasterization State", ImGuiTreeNodeFlags_DefaultOpen ) )
    {
      ImGui::Checkbox( "Rasterizer Discard", &state::rasterizerDiscardEnable );

      // Cull mode
      int          cullMode         = static_cast<int>( state::cullMode );
      const char * cullModeLabels[] = { "None", "Front", "Back", "Front and Back" };
      if ( ImGui::Combo( "Cull Mode", &cullMode, cullModeLabels, IM_ARRAYSIZE( cullModeLabels ) ) )
      {
        switch ( cullMode )
        {
          case 0: state::cullMode = vk::CullModeFlagBits::eNone; break;
          case 1: state::cullMode = vk::CullModeFlagBits::eFront; break;
          case 2: state::cullMode = vk::CullModeFlagBits::eBack; break;
          case 3: state::cullMode = vk::CullModeFlagBits::eFrontAndBack; break;
        }
      }

      // Front face
      int          frontFace    = static_cast<int>( state::frontFace );
      const char * frontFaces[] = { "CounterClockwise", "Clockwise" };
      if ( ImGui::Combo( "Front Face", &frontFace, frontFaces, IM_ARRAYSIZE( frontFaces ) ) )
      {
        state::frontFace = static_cast<vk::FrontFace>( frontFace );
      }

      // Polygon mode
      int          polygonMode    = static_cast<int>( state::polygonMode );
      const char * polygonModes[] = { "Fill", "Line", "Point" };
      if ( ImGui::Combo( "Polygon Mode", &polygonMode, polygonModes, IM_ARRAYSIZE( polygonModes ) ) )
      {
        switch ( polygonMode )
        {
          case 0: state::polygonMode = vk::PolygonMode::eFill; break;
          case 1: state::polygonMode = vk::PolygonMode::eLine; break;
          case 2: state::polygonMode = vk::PolygonMode::ePoint; break;
        }
      }
      ImGui::SliderFloat( "Line Width", &state::lineWidth, 0.0f, 10.0f, "%.2f", 1.0f );
    }

    // Depth/Stencil
    if ( ImGui::CollapsingHeader( "Depth/Stencil State", ImGuiTreeNodeFlags_DefaultOpen ) )
    {
      ImGui::Checkbox( "Depth Test Enable", &state::depthTestEnable );
      ImGui::Checkbox( "Depth Write Enable", &state::depthWriteEnable );

      // Depth Compare Op
      int          compareOp       = static_cast<int>( state::depthCompareOp );
      const char * compareLabels[] = { "Never", "Less", "Equal", "LessOrEqual", "Greater", "NotEqual", "GreaterOrEqual", "Always" };
      if ( ImGui::Combo( "Depth Compare Op", &compareOp, compareLabels, IM_ARRAYSIZE( compareLabels ) ) )
      {
        state::depthCompareOp = static_cast<vk::CompareOp>( compareOp );
      }
      ImGui::Checkbox( "Depth Bias Enable", &state::depthBiasEnable );
      ImGui::Checkbox( "Stencil Test Enable", &state::stencilTestEnable );
    }

    // Primitive State
    if ( ImGui::CollapsingHeader( "Primitive State", ImGuiTreeNodeFlags_DefaultOpen ) )
    {
      int          topology        = static_cast<int>( state::primitiveTopology );
      const char * topologyNames[] = { "PointList",
                                       "LineList",
                                       "LineStrip",
                                       "TriangleList",
                                       "TriangleStrip",
                                       "TriangleFan",
                                       "LineListWithAdjacency",
                                       "LineStripWithAdjacency",
                                       "TriangleListWithAdjacency",
                                       "TriangleStripWithAdjacency",
                                       "PatchList" };
      if ( ImGui::Combo( "Topology", &topology, topologyNames, 6 /* only up to triangle fan for usual usage */ ) )
      {
        state::primitiveTopology = static_cast<vk::PrimitiveTopology>( topology );
      }
      ImGui::Checkbox( "Primitive Restart Enable", &state::primitiveRestartEnable );
    }

    // Multisample State
    if ( ImGui::CollapsingHeader( "Multisample State", ImGuiTreeNodeFlags_DefaultOpen ) )
    {
      int sampleCount = 0;
      switch ( state::rasterizationSamples )
      {
        case vk::SampleCountFlagBits::e1 : sampleCount = 0; break;
        case vk::SampleCountFlagBits::e2 : sampleCount = 1; break;
        case vk::SampleCountFlagBits::e4 : sampleCount = 2; break;
        case vk::SampleCountFlagBits::e8 : sampleCount = 3; break;
        case vk::SampleCountFlagBits::e16: sampleCount = 4; break;
        case vk::SampleCountFlagBits::e32: sampleCount = 5; break;
        case vk::SampleCountFlagBits::e64: sampleCount = 6; break;
        default                          : break;
      }
      const char * sampleLabels[] = { "1x", "2x", "4x", "8x", "16x", "32x", "64x" };
      if ( ImGui::Combo( "Samples", &sampleCount, sampleLabels, IM_ARRAYSIZE( sampleLabels ) ) )
      {
        switch ( sampleCount )
        {
          case 0: state::rasterizationSamples = vk::SampleCountFlagBits::e1; break;
          case 1: state::rasterizationSamples = vk::SampleCountFlagBits::e2; break;
          case 2: state::rasterizationSamples = vk::SampleCountFlagBits::e4; break;
          case 3: state::rasterizationSamples = vk::SampleCountFlagBits::e8; break;
          case 4: state::rasterizationSamples = vk::SampleCountFlagBits::e16; break;
          case 5: state::rasterizationSamples = vk::SampleCountFlagBits::e32; break;
          case 6: state::rasterizationSamples = vk::SampleCountFlagBits::e64; break;
        }
      }
    }

    ImGui::End();
  }
}  // namespace ui
