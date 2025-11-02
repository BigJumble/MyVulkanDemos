#pragma once
#include "vulkan/vulkan.hpp"

#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>


// clang-format off
namespace state {

    //=========================================================
    // Vulkan present mode selection
    //=========================================================

    inline std::vector<vk::PresentModeKHR> availablePresentModes = { };
    inline vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
    
    //=========================================================
    // Graphics pipeline
    //=========================================================

    //=========================================================
    // Rasterization and Depth/Stencil Test States
    //=========================================================

    // Rasterization state
    inline bool rasterizerDiscardEnable = false;
    inline vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eNone;
    inline vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
    inline vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
    inline float lineWidth = 1.0f;

    // Depth/Stencil test state
    inline bool depthTestEnable = true;
    inline bool depthWriteEnable = true;
    inline vk::CompareOp depthCompareOp = vk::CompareOp::eLess;
    inline bool depthBiasEnable = false;
    inline bool stencilTestEnable = false;

    // Primitive state
    inline vk::PrimitiveTopology primitiveTopology = vk::PrimitiveTopology::eTriangleList;
    inline bool primitiveRestartEnable = false;

    // Multisample state
    // inline vk::SampleCountFlagBits rasterizationSamples = vk::SampleCountFlagBits::e1;

    //=========================================================
    // User Input
    //=========================================================

    inline glm::vec3 cameraPosition = glm::vec3( 0.0f, 0.0f, 0.0f );
    inline glm::vec2 cameraRotation = glm::vec2( 0.0f, 0.0f );

}  // namespace state