#pragma once
#include "structs.hpp"

#include <glm/glm.hpp>
#include <set>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace eeng
{
  // clang-format off
    namespace state {

        inline constexpr std::string_view AppName    = "MyApp";
        inline constexpr std::string_view EngineName = "MyEngine";
        
        constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
        
        inline bool framebufferResized = false;
        inline vk::Extent2D screenSize = {1280,720};

        // inline vk::raii::PhysicalDevice physicalDevice = nullptr;
        // inline core::DeviceBundle deviceBundle;
        
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

        // Enum for a subset of GLFW keys: W, A, S, D, Shift, Ctrl, Space


        inline std::set<eeng::Key> keysPressed;
        inline std::set<eeng::Key> keysDown;
        inline std::set<eeng::Key> keysUp;

        
        inline glm::vec2 cursorDelta;

        //=========================================================
        // Game state
        //=========================================================

        inline bool imguiMode = false;
        inline glm::vec3 cameraPosition = glm::vec3( 0.0f, 0.0f, 0.0f );
        inline glm::vec2 cameraRotation = glm::vec2( 0.0f, 0.0f );
        inline float cameraZoom = 1.0f;
    }  // namespace state
}