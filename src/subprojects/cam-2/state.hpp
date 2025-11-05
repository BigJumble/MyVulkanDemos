#pragma once
#include "vulkan/vulkan.hpp"

#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include "bootstrap.hpp"
#include <glm/glm.hpp>


// clang-format off
namespace state {

    inline bool framebufferResized = false;
    inline vk::Extent2D screenSize = {0,0};
    inline vk::raii::PhysicalDevice physicalDevice = nullptr;
    inline core::DeviceBundle deviceBundle;
    //=========================================================
    // Vulkan present mode selection
    //=========================================================

    inline std::vector<vk::PresentModeKHR> availablePresentModes = { };
    inline vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
    
    //=========================================================
    // Graphics pipeline
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

    inline bool fpvMode = false;
    inline glm::vec3 cameraPosition = glm::vec3( 0.0f, 0.0f, 0.0f );
    inline glm::vec2 cameraRotation = glm::vec2( 0.0f, 0.0f );
    inline float cameraZoom = 1.0f;
    inline float lastX = 0.0f;
    inline float lastY = 0.0f;
    inline bool cursorInWindow = true;
    inline int windowWidth = 0;
    inline int windowHeight = 0;

}  // namespace state