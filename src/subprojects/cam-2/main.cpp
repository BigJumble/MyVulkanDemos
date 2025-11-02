
#include "bootstrap.hpp"
#include "data.hpp"
#include "features.hpp"
#include "helper.hpp"
#include "init.hpp"
#include "state.hpp"
#include "vulkan/vulkan.hpp"

#include <vulkan/vulkan_raii.hpp>

#define VMA_IMPLEMENTATION

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "ui.hpp"

#include <print>
#include <vk_mem_alloc.h>

static void recordCommandBuffer(
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
  if(state::polygonMode == vk::PolygonMode::eLine) {
    cmd.setLineWidth(state::lineWidth);
  }
  cmd.setRasterizationSamplesEXT( state::rasterizationSamples );


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

  // Render ImGui on top
  ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), *cmd );

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

static void framebufferResizeCallback( GLFWwindow * win, int, int )
{
  auto * resized = static_cast<bool *>( glfwGetWindowUserPointer( win ) );
  if ( resized )
  {
    *resized = true;
  }
}

static void recreateSwapchain(
  core::DisplayBundle &        displayBundle,
  vk::raii::PhysicalDevice &   physicalDevice,
  core::DeviceBundle &         deviceBundle,
  core::SwapchainBundle &      swapchainBundle,
  core::QueueFamilyIndices &   queueFamilyIndices,
  VmaAllocator &               allocator,
  init::raii::DepthResources & depthResources )
{
  int width = 0, height = 0;
  do
  {
    glfwGetFramebufferSize( displayBundle.window, &width, &height );
    glfwPollEvents();
  } while ( width == 0 || height == 0 );

  deviceBundle.device.waitIdle();

  core::SwapchainBundle old = std::move( swapchainBundle );
  swapchainBundle           = core::createSwapchain(
    physicalDevice,
    deviceBundle.device,
    displayBundle.surface,
    vk::Extent2D{ static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) },
    queueFamilyIndices,
    &old.swapchain );

  // Recreate depth resources with new extent
  depthResources = init::raii::DepthResources( deviceBundle.device, allocator, swapchainBundle.extent );

  // No need to recreate per-frame semaphores - they're independent of swapchain
}

int main()
{
  /* VULKAN_HPP_KEY_START */
  // isDebug( std::println( "LOADING UP CAMERA INSTANCING EXAMPLE!\n" ); );
  try
  {
    vk::raii::Context context;

    vk::raii::Instance instance( context, init::createInfo );

    vk::raii::PhysicalDevices physicalDevices( instance );

    vk::raii::PhysicalDevice physicalDevice = core::selectPhysicalDevice( physicalDevices );

    core::DisplayBundle displayBundle( instance, init::AppName.data(), vk::Extent2D( 1280, 720 ) );

    state::availablePresentModes = physicalDevice.getSurfacePresentModesKHR( displayBundle.surface );

    core::QueueFamilyIndices queueFamilyIndices = core::findQueueFamilies( physicalDevice, displayBundle.surface );

    core::DeviceBundle deviceBundle = core::createDeviceWithQueues( physicalDevice, queueFamilyIndices, cfg::enabledFeaturesChain, cfg::getRequiredExtensions );

    core::SwapchainBundle swapchainBundle =
      core::createSwapchain( physicalDevice, deviceBundle.device, displayBundle.surface, displayBundle.extent, queueFamilyIndices );



    init::raii::Allocator allocator( instance, physicalDevice, deviceBundle.device );

    // Create depth resources
    init::raii::DepthResources depthResources( deviceBundle.device, allocator, swapchainBundle.extent );

    //=========================================================
    // ImGui setup
    //=========================================================

    std::array<vk::DescriptorPoolSize, 11> imguiPoolSizes = { vk::DescriptorPoolSize{ vk::DescriptorType::eSampler, 1000 },
                                                              vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 1000 },
                                                              vk::DescriptorPoolSize{ vk::DescriptorType::eSampledImage, 1000 },
                                                              vk::DescriptorPoolSize{ vk::DescriptorType::eStorageImage, 1000 },
                                                              vk::DescriptorPoolSize{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
                                                              vk::DescriptorPoolSize{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
                                                              vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 1000 },
                                                              vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, 1000 },
                                                              vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
                                                              vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
                                                              vk::DescriptorPoolSize{ vk::DescriptorType::eInputAttachment, 1000 } };

    vk::DescriptorPoolCreateInfo imguiPoolInfo{};
    imguiPoolInfo.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    imguiPoolInfo.maxSets       = 1000 * static_cast<uint32_t>( imguiPoolSizes.size() );
    imguiPoolInfo.poolSizeCount = static_cast<uint32_t>( imguiPoolSizes.size() );
    imguiPoolInfo.pPoolSizes    = imguiPoolSizes.data();
    vk::raii::DescriptorPool imguiDescriptorPool{ deviceBundle.device, imguiPoolInfo };

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan( displayBundle.window, true );

    ImGui_ImplVulkan_InitInfo imguiInitInfo{};
    imguiInitInfo.Instance            = *instance;
    imguiInitInfo.PhysicalDevice      = *physicalDevice;
    imguiInitInfo.Device              = *deviceBundle.device;
    imguiInitInfo.QueueFamily         = queueFamilyIndices.graphicsFamily.value();
    imguiInitInfo.Queue               = *deviceBundle.graphicsQueue;
    imguiInitInfo.DescriptorPool      = *imguiDescriptorPool;
    imguiInitInfo.RenderPass          = VK_NULL_HANDLE;
    imguiInitInfo.MinImageCount       = static_cast<uint32_t>( swapchainBundle.images.size() );
    imguiInitInfo.ImageCount          = static_cast<uint32_t>( swapchainBundle.images.size() );
    imguiInitInfo.MSAASamples         = VK_SAMPLE_COUNT_1_BIT;
    imguiInitInfo.UseDynamicRendering = true;

    // Set up dynamic rendering info for ImGui
    vk::PipelineRenderingCreateInfoKHR pipelineRenderingInfo{};
    pipelineRenderingInfo.colorAttachmentCount    = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &swapchainBundle.imageFormat;
    pipelineRenderingInfo.depthAttachmentFormat = depthResources.depthFormat;

    imguiInitInfo.PipelineRenderingCreateInfo = pipelineRenderingInfo;
    ImGui_ImplVulkan_Init( &imguiInitInfo );

    //=========================================================
    // Vulkan setup
    //=========================================================

    init::raii::ShaderBundle shaderBundle(
      deviceBundle.device,
      { "triangle.vert" },
      { "triangle.frag" },
      vk::PushConstantRange{ vk::ShaderStageFlagBits::eVertex, 0, sizeof( data::PushConstants ) } );

    VkDeviceSize bufferSize = sizeof( data::triangleVertices );

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size               = bufferSize;
    bufferInfo.usage              = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VkBuffer          vertexBuffer;
    VmaAllocation     vertexBufferAllocation;
    VmaAllocationInfo vertexBufferAllocInfo;

    vmaCreateBuffer( allocator, &bufferInfo, &allocInfo, &vertexBuffer, &vertexBufferAllocation, &vertexBufferAllocInfo );

    // Copy vertex data to buffer (already mapped)
    memcpy( vertexBufferAllocInfo.pMappedData, data::triangleVertices.data(), static_cast<size_t>( bufferSize ) );

    uint32_t     instanceCount      = static_cast<uint32_t>( data::instancesPos.size() );
    VkDeviceSize instanceBufferSize = sizeof( data::InstanceData ) * data::instancesPos.size();

    VkBufferCreateInfo instanceBufferInfo = {};
    instanceBufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    instanceBufferInfo.size               = instanceBufferSize;
    instanceBufferInfo.usage              = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    instanceBufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer          instanceBuffer;
    VmaAllocation     instanceBufferAllocation;
    VmaAllocationInfo instanceBufferAllocInfo;

    vmaCreateBuffer( allocator, &instanceBufferInfo, &allocInfo, &instanceBuffer, &instanceBufferAllocation, &instanceBufferAllocInfo );

    // Copy instance data to buffer
    memcpy( instanceBufferAllocInfo.pMappedData, data::instancesPos.data(), static_cast<size_t>( instanceBufferSize ) );

    vk::CommandPoolCreateInfo cmdPoolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndices.graphicsFamily.value() };
    vk::raii::CommandPool     commandPool{ deviceBundle.device, cmdPoolInfo };

    // Frames in flight - independent of swapchain image count
    constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    vk::CommandBufferAllocateInfo cmdInfo{ commandPool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT };
    vk::raii::CommandBuffers      cmds{ deviceBundle.device, cmdInfo };

    // Create per-frame binary semaphores for image acquisition and presentation
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence>     presentFences;

    imageAvailableSemaphores.reserve( MAX_FRAMES_IN_FLIGHT );
    renderFinishedSemaphores.reserve( MAX_FRAMES_IN_FLIGHT );
    presentFences.reserve( MAX_FRAMES_IN_FLIGHT );

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
      imageAvailableSemaphores.emplace_back( deviceBundle.device, vk::SemaphoreCreateInfo{} );
      renderFinishedSemaphores.emplace_back( deviceBundle.device, vk::SemaphoreCreateInfo{} );
      presentFences.emplace_back( deviceBundle.device, vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled } );
    }

    bool framebufferResized = false;
    // Set a pointer to framebufferResized so the resize callback can modify it
    glfwSetWindowUserPointer( displayBundle.window, &framebufferResized );
    // Register a callback function to be called when the window is resized
    glfwSetFramebufferSizeCallback( displayBundle.window, framebufferResizeCallback );

    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    size_t currentFrame = 0;

    while ( !glfwWindowShouldClose( displayBundle.window ) )
    {
      glfwPollEvents();

      if ( framebufferResized )
      {
        framebufferResized = false;
        recreateSwapchain( displayBundle, physicalDevice, deviceBundle, swapchainBundle, queueFamilyIndices, allocator.allocator, depthResources );
        continue;
      }

      // Start ImGui frame
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      ui::renderStatsWindow();
      ui::renderPresentModeWindow();
      ui::renderPipelineStateWindow();

      ImGui::Render();

      try
      {
        // Get synchronization objects for current frame in flight
        auto & imageAvailable = imageAvailableSemaphores[currentFrame];
        auto & renderFinished = renderFinishedSemaphores[currentFrame];
        auto & presentFence   = presentFences[currentFrame];

        // Wait for the presentation fence from previous use of this frame slot
        // Wait for the present fence to be signaled before reusing this frame slot
        (void)deviceBundle.device.waitForFences( { *presentFence }, VK_TRUE, UINT64_MAX );

        // Acquire next swapchain image using the imageAvailable semaphore
        auto acquire = swapchainBundle.swapchain.acquireNextImage( UINT64_MAX, *imageAvailable, nullptr );
        if ( acquire.result == vk::Result::eErrorOutOfDateKHR )
        {
          throw std::runtime_error( "acquire.result: " + std::to_string( static_cast<int>( acquire.result ) ) );
        }
        uint32_t imageIndex = acquire.value;

        // Only reset the fence after successful image acquisition to prevent deadlock on exception
        deviceBundle.device.resetFences( { *presentFence } );
        // std::println( "imageIndex: {}", imageIndex );
        // Record command buffer for this frame
        auto & cmd = cmds[currentFrame];
        recordCommandBuffer( cmd, shaderBundle, swapchainBundle, imageIndex, vertexBuffer, instanceBuffer, instanceCount, depthResources );

        // Submit command buffer waiting on imageAvailable, signal renderFinished and timeline
        // uint64_t renderCompleteValue = ++currentTimelineValue;

        std::array<vk::SemaphoreSubmitInfo, 1> waitSemaphoreInfos = {
          vk::SemaphoreSubmitInfo{}.setSemaphore( *imageAvailable ).setStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput )
        };

        std::array<vk::SemaphoreSubmitInfo, 1> signalSemaphoreInfos = {
          vk::SemaphoreSubmitInfo{}.setSemaphore( *renderFinished ).setStageMask( vk::PipelineStageFlagBits2::eAllCommands ),
          // vk::SemaphoreSubmitInfo{}.setSemaphore( *syncSemaphore ).setValue( renderCompleteValue ).setStageMask( vk::PipelineStageFlagBits2::eAllCommands )
        };

        vk::CommandBufferSubmitInfo cmdBufferInfo{};
        cmdBufferInfo.setCommandBuffer( *cmd );

        vk::SubmitInfo2 submitInfo{};
        submitInfo.setCommandBufferInfoCount( 1 )
          .setPCommandBufferInfos( &cmdBufferInfo )
          .setWaitSemaphoreInfos( waitSemaphoreInfos )
          .setSignalSemaphoreInfos( signalSemaphoreInfos );

        deviceBundle.graphicsQueue.submit2( submitInfo );

        // Dynamic vsync: Use FIFO for vsync, or switch to immediate/mailbox for uncapped
        // For now, using FIFO (vsync enabled). Change to eImmediate or eMailbox for no vsync

        vk::SwapchainPresentScalingCreateInfoEXT presentScalingInfo{};
        presentScalingInfo.setPresentGravityX( vk::PresentGravityFlagBitsKHR::eCentered );
        presentScalingInfo.setPresentGravityY( vk::PresentGravityFlagBitsKHR::eCentered );
        presentScalingInfo.setScalingBehavior( vk::PresentScalingFlagBitsKHR::eAspectRatioStretch );

        vk::SwapchainPresentModeInfoEXT presentModeInfo{};
        presentModeInfo.setSwapchainCount( 1 );
        presentModeInfo.setPPresentModes( &state::presentMode );

        vk::SwapchainPresentFenceInfoEXT presentFenceInfo{};
        presentFenceInfo.setSwapchainCount( 1 ).setPFences( &*presentFence );
        presentFenceInfo.setPNext( &presentModeInfo );

        vk::PresentInfoKHR presentInfo{};
        presentInfo.setPNext( &presentFenceInfo )
          .setWaitSemaphoreCount( 1 )
          .setPWaitSemaphores( &*renderFinished )
          .setSwapchainCount( 1 )
          .setPSwapchains( &*swapchainBundle.swapchain )
          .setPImageIndices( &imageIndex );

        auto presentRes = deviceBundle.graphicsQueue.presentKHR( presentInfo );

        if ( presentRes == vk::Result::eSuboptimalKHR || presentRes == vk::Result::eErrorOutOfDateKHR )
        {
          throw std::runtime_error( "presentRes: " + std::to_string( static_cast<int>( presentRes ) ) );
        }

        currentFrame = ( currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
      }
      catch ( std::exception const & err )
      {
        isDebug( std::println( "Frame rendering exception (recreating swapchain): {}", err.what() ) );
        recreateSwapchain( displayBundle, physicalDevice, deviceBundle, swapchainBundle, queueFamilyIndices, allocator.allocator, depthResources );
        continue;
      }
    }

    deviceBundle.device.waitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // Cleanup VMA resources;
    vmaDestroyBuffer( allocator, vertexBuffer, vertexBufferAllocation );
    vmaDestroyBuffer( allocator, instanceBuffer, instanceBufferAllocation );
    // vmaDestroyAllocator( allocator );
  }

  catch ( vk::SystemError & err )
  {
    std::println( "vk::SystemError: {}", err.what() );
    exit( -1 );
  }
  catch ( std::exception & err )
  {
    std::println( "vk::exception: {}", err.what() );
    exit( -1 );
  }
  catch ( ... )
  {
    std::println( "unknown error" );
    exit( -1 );
  }

  /* VULKAN_HPP_KEY_END */

  return 0;
}
