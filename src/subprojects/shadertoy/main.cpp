#include "bootstrap.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "rendering.hpp"
#include "settings.hpp"
#include "swapchain_utils.hpp"
#include "ui.hpp"

#include <print>

constexpr std::string_view AppName    = "MyApp";
constexpr std::string_view EngineName = "MyEngine";

int main()
{
  /* VULKAN_HPP_KEY_START */
  isDebug( std::println( "LOADING UP SYNCTEST EXAMPLE!\n" ); );
  try
  {
    vk::raii::Context context;

    vk::raii::Instance instance = core::createInstance( context, std::string( AppName ), std::string( EngineName ) );

    vk::raii::PhysicalDevices physicalDevices( instance );

    vk::raii::PhysicalDevice physicalDevice = core::selectPhysicalDevice( physicalDevices );

    core::DisplayBundle displayBundle( instance, "MyEngine", vk::Extent2D( 1280, 720 ) );

    core::QueueFamilyIndices queueFamilyIndices = core::findQueueFamilies( physicalDevice, displayBundle.surface );

    core::DeviceBundle deviceBundle = core::createDeviceWithQueues( physicalDevice, queueFamilyIndices );

    core::SwapchainBundle swapchainBundle = core::createSwapchain( physicalDevice, deviceBundle.device, displayBundle.surface, displayBundle.extent, queueFamilyIndices );

    vk::CommandPoolCreateInfo cmdPoolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndices.graphicsFamily.value() };
    vk::raii::CommandPool     commandPool{ deviceBundle.device, cmdPoolInfo };

    // Create ImGui descriptor pool
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

    imguiInitInfo.PipelineRenderingCreateInfo = pipelineRenderingInfo;
    ImGui_ImplVulkan_Init( &imguiInitInfo );

    // Frames in flight - independent of swapchain image count
    constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    vk::CommandBufferAllocateInfo cmdInfo{ commandPool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT };
    vk::raii::CommandBuffers      cmds{ deviceBundle.device, cmdInfo };

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
    glfwSetFramebufferSizeCallback( displayBundle.window, swapchain_utils::framebufferResizeCallback );

    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    size_t currentFrame = 0;

    // Resource Manager state
    ui::ResourceManagerState resourceManagerState;

    // Main Loop state
    ui::MainLoopState mainLoopState;

    while ( !glfwWindowShouldClose( displayBundle.window ) )
    {
      glfwPollEvents();

      if ( framebufferResized )
      {
        framebufferResized = false;
        swapchain_utils::recreateSwapchain( displayBundle, physicalDevice, deviceBundle, swapchainBundle, queueFamilyIndices );
        continue;
      }

      // Start ImGui frame
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      // Render UI windows
      ui::renderStatsWindow();
      ui::renderResourceManagerWindow( resourceManagerState );
      ui::renderMainLoopWindow( mainLoopState, resourceManagerState );

      // ImGui::ShowDemoWindow();

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
          throw acquire.result;
        }
        uint32_t imageIndex = acquire.value;

        // Only reset the fence after successful image acquisition to prevent deadlock on exception
        deviceBundle.device.resetFences( { *presentFence } );
        // std::println( "imageIndex: {}", imageIndex );
        // Record command buffer for this frame
        auto & cmd = cmds[currentFrame];
        rendering::recordCommandBuffer( cmd, swapchainBundle, imageIndex );

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

        vk::SwapchainPresentFenceInfoEXT presentFenceInfo{};
        presentFenceInfo.setSwapchainCount( 1 ).setPFences( &*presentFence );

        vk::PresentInfoKHR presentInfo{};
        presentInfo.setPNext( &presentFenceInfo )
          .setWaitSemaphoreCount( 1 )
          .setPWaitSemaphores( &*renderFinished )
          .setSwapchainCount( 1 )
          .setPSwapchains( &*swapchainBundle.swapchain )
          .setPImageIndices( &imageIndex );

        auto presentRes = deviceBundle.presentQueue.presentKHR( presentInfo );

        if ( presentRes == vk::Result::eSuboptimalKHR || presentRes == vk::Result::eErrorOutOfDateKHR )
        {
          throw presentRes;
        }

        currentFrame = ( currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
      }
      catch ( std::exception const & err )
      {
        isDebug( std::println( "Frame rendering exception (recreating swapchain): {}", err.what() ) );
        swapchain_utils::recreateSwapchain( displayBundle, physicalDevice, deviceBundle, swapchainBundle, queueFamilyIndices );
        continue;
      }
    }

    // Cleanup ImGui
    deviceBundle.device.waitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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
