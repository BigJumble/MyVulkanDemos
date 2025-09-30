#include "helper.hpp"
#include "settings.hpp"
#include <print>
#include <array>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

static std::string AppName    = "01_InitInstance";
static std::string EngineName = "Vulkan.hpp";

int main()
{
  /* VULKAN_HPP_KEY_START */

  try
  {
    // the very beginning: instantiate a context
    vk::raii::Context context;

    // create an Instance
    vk::raii::Instance instance( context, core::createInstanceCreateInfo( AppName, EngineName, {}, core::InstanceExtensions ) );

    isDebug( vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger( instance, core::createDebugUtilsMessengerCreateInfo() ); );

    vk::raii::PhysicalDevices physicalDevices( instance );

    vk::raii::PhysicalDevice physicalDevice = core::selectPhysicalDevice( physicalDevices );

    core::DisplayBundle Display( instance, "MyEngine", vk::Extent2D( 1280, 720 ) );

    core::QueueFamilyIndices indices = core::findQueueFamilies( physicalDevice, Display.surface );

    core::DeviceBundle deviceBundle = core::createDeviceWithQueues( physicalDevice, indices );

    core::SwapchainBundle swapchain = core::createSwapchain( physicalDevice, deviceBundle.device, Display.surface, Display.extent, indices );

    isDebug(
      std::println(
        "Swapchain created: {} images, format {}, extent {}x{}",
        swapchain.images.size(),
        vk::to_string( swapchain.imageFormat ),
        swapchain.extent.width,
        swapchain.extent.height ); );

    // Load fullscreen quad shaders
    std::vector<uint32_t> vertShaderCode = core::readSpirvFile( "shaders/fullscreen_quad.vert.spv" );
    std::vector<uint32_t> fragShaderCode = core::readSpirvFile( "shaders/textured.frag.spv" );

    vk::raii::ShaderModule vertShaderModule = core::createShaderModule( deviceBundle.device, vertShaderCode );
    vk::raii::ShaderModule fragShaderModule = core::createShaderModule( deviceBundle.device, fragShaderCode );

    // Create a render pass matching the swapchain image format
    vk::raii::RenderPass renderPass = core::createRenderPass( deviceBundle.device, swapchain.imageFormat );

    // Create a graphics descriptor set layout (binding 0: combined image sampler)
    vk::DescriptorSetLayoutBinding gfxBinding{ 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment };
    vk::DescriptorSetLayoutCreateInfo gfxDslCi{}; gfxDslCi.bindingCount = 1; gfxDslCi.pBindings = &gfxBinding;
    vk::raii::DescriptorSetLayout gfxDsl{ deviceBundle.device, gfxDslCi };

    // Create a pipeline layout for graphics with that descriptor set layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{}; pipelineLayoutInfo.setSetLayouts( *gfxDsl );
    vk::raii::PipelineLayout pipelineLayout{ deviceBundle.device, pipelineLayoutInfo };

    // Create the graphics pipeline using the loaded shader modules
    vk::raii::Pipeline graphicsPipeline = core::createGraphicsPipeline( deviceBundle.device, renderPass, pipelineLayout, swapchain.extent, vertShaderModule, fragShaderModule );

    // Create framebuffers for each swapchain image view
    std::vector<vk::raii::Framebuffer> framebuffers = core::createFramebuffers( deviceBundle.device, renderPass, swapchain.extent, swapchain.imageViews );

    // Create command pool and buffers (one per framebuffer)
    core::CommandResources commandResources = core::createCommandResources( deviceBundle.device, deviceBundle.indices.graphicsFamily, framebuffers.size() );

    // Record commands initially (will re-record each frame anyway)
    core::recordTriangleCommands( commandResources.buffers, renderPass, framebuffers.data(), framebuffers.size(), swapchain.extent, graphicsPipeline );

    // --- Compute resources for Game of Life ---
    auto createStorageImage = [&](vk::Extent2D extent) {
      vk::ImageCreateInfo imgInfo{};
      imgInfo.imageType = vk::ImageType::e2D;
      imgInfo.extent = vk::Extent3D{ extent.width, extent.height, 1 };
      imgInfo.mipLevels = 1;
      imgInfo.arrayLayers = 1;
      imgInfo.format = vk::Format::eR8G8B8A8Unorm;
      imgInfo.tiling = vk::ImageTiling::eOptimal;
      imgInfo.initialLayout = vk::ImageLayout::eUndefined;
      imgInfo.usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;
      imgInfo.sharingMode = vk::SharingMode::eExclusive;
      imgInfo.samples = vk::SampleCountFlagBits::e1;
      vk::raii::Image image{ deviceBundle.device, imgInfo };

      vk::MemoryRequirements memReq = image.getMemoryRequirements();
      vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();
      uint32_t typeIndex = UINT32_MAX;
      for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((memReq.memoryTypeBits & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) { typeIndex = i; break; }
      }
      if (typeIndex == UINT32_MAX) { throw std::runtime_error("No suitable memory type for storage image"); }
      vk::MemoryAllocateInfo alloc{ memReq.size, typeIndex };
      vk::raii::DeviceMemory mem{ deviceBundle.device, alloc };
      image.bindMemory(*mem, 0);

      vk::ImageViewCreateInfo iv{};
      iv.image = *image;
      iv.viewType = vk::ImageViewType::e2D;
      iv.format = vk::Format::eR8G8B8A8Unorm;
      iv.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
      iv.subresourceRange.baseMipLevel = 0; iv.subresourceRange.levelCount = 1;
      iv.subresourceRange.baseArrayLayer = 0; iv.subresourceRange.layerCount = 1;
      vk::raii::ImageView view{ deviceBundle.device, iv };
      return std::tuple<vk::raii::Image, vk::raii::DeviceMemory, vk::raii::ImageView>(std::move(image), std::move(mem), std::move(view));
    };

    // Fixed 256x256 state images for simplicity
    vk::Extent2D simExtent{ 256, 256 };
    auto [stateA, stateAMem, stateAView] = createStorageImage(simExtent);
    auto [stateB, stateBMem, stateBView] = createStorageImage(simExtent);

    // Descriptor set layouts (read: sampler2D, write: storage image)
    vk::DescriptorSetLayoutBinding b0{ 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute };
    vk::DescriptorSetLayoutBinding b1{ 1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute };
    std::array<vk::DescriptorSetLayoutBinding,2> bindings{ b0, b1 };
    vk::DescriptorSetLayoutCreateInfo dslCi{}; dslCi.bindingCount = bindings.size(); dslCi.pBindings = bindings.data();
    vk::raii::DescriptorSetLayout golDsl{ deviceBundle.device, dslCi };
    vk::raii::DescriptorSetLayout blitDsl{ deviceBundle.device, dslCi };

    // Pipeline layouts
    vk::PipelineLayoutCreateInfo plCiGol{}; plCiGol.setSetLayouts(*golDsl);
    vk::raii::PipelineLayout golPipelineLayout{ deviceBundle.device, plCiGol };
    vk::PipelineLayoutCreateInfo plCiBlit{}; plCiBlit.setSetLayouts(*blitDsl);
    vk::raii::PipelineLayout blitPipelineLayout{ deviceBundle.device, plCiBlit };

    // Sampler
    vk::SamplerCreateInfo sci{}; sci.magFilter = vk::Filter::eNearest; sci.minFilter = vk::Filter::eNearest; sci.mipmapMode = vk::SamplerMipmapMode::eNearest; sci.addressModeU = vk::SamplerAddressMode::eClampToEdge; sci.addressModeV = vk::SamplerAddressMode::eClampToEdge; sci.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    vk::raii::Sampler sampler{ deviceBundle.device, sci };

    // Compute shaders and pipelines
    auto spvSeed = core::readSpirvFile("shaders/seed.comp.spv");
    auto spvGol  = core::readSpirvFile("shaders/game_of_life.comp.spv");
    auto spvBlit = core::readSpirvFile("shaders/blit_to_swapchain.comp.spv");
    vk::raii::ShaderModule seedModule = core::createShaderModule(deviceBundle.device, spvSeed);
    vk::raii::ShaderModule golModule  = core::createShaderModule(deviceBundle.device, spvGol);
    vk::raii::ShaderModule blitModule = core::createShaderModule(deviceBundle.device, spvBlit);
    auto makeCompute = [&](vk::raii::ShaderModule &m, vk::raii::PipelineLayout &pl){
      vk::PipelineShaderStageCreateInfo st{}; st.stage = vk::ShaderStageFlagBits::eCompute; st.module = *m; st.pName = "main";
      vk::ComputePipelineCreateInfo ci{}; ci.stage = st; ci.layout = *pl; return vk::raii::Pipeline(deviceBundle.device, nullptr, ci);
    };
    vk::raii::Pipeline seedPipeline = makeCompute(seedModule, golPipelineLayout);
    vk::raii::Pipeline golPipeline  = makeCompute(golModule, golPipelineLayout);

    // Descriptor pool and sets
    std::array<vk::DescriptorPoolSize,2> poolSizes = { vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 32 }, vk::DescriptorPoolSize{ vk::DescriptorType::eStorageImage, 32 } };
    vk::DescriptorPoolCreateInfo poolCi{}; poolCi.maxSets = 64; poolCi.poolSizeCount = poolSizes.size(); poolCi.pPoolSizes = poolSizes.data();
    vk::raii::DescriptorPool descPool{ deviceBundle.device, poolCi };

    std::array<vk::DescriptorSetLayout,2> golLayouts{ *golDsl, *golDsl };
    vk::DescriptorSetAllocateInfo allocGol{ *descPool, static_cast<uint32_t>(golLayouts.size()), golLayouts.data() };
    vk::raii::DescriptorSets golSets{ deviceBundle.device, allocGol };
    // Graphics descriptor sets (sample from A and B)
    std::array<vk::DescriptorSetLayout,2> gfxLayouts{ *gfxDsl, *gfxDsl };
    vk::DescriptorSetAllocateInfo allocGfx{ *descPool, static_cast<uint32_t>(gfxLayouts.size()), gfxLayouts.data() };
    vk::raii::DescriptorSets gfxSets{ deviceBundle.device, allocGfx };

    // Update descriptor sets
    vk::DescriptorImageInfo readA{}; readA.sampler = *sampler; readA.imageView = *stateAView; readA.imageLayout = vk::ImageLayout::eGeneral;
    vk::DescriptorImageInfo writeB{}; writeB.imageView = *stateBView; writeB.imageLayout = vk::ImageLayout::eGeneral;
    // GOL descriptor set 0: read A, write B
    vk::WriteDescriptorSet golWrite0{}; golWrite0.dstSet = *golSets[0]; golWrite0.dstBinding = 0; golWrite0.descriptorType = vk::DescriptorType::eCombinedImageSampler; golWrite0.descriptorCount = 1; golWrite0.pImageInfo = &readA;
    vk::WriteDescriptorSet golWrite1{}; golWrite1.dstSet = *golSets[0]; golWrite1.dstBinding = 1; golWrite1.descriptorType = vk::DescriptorType::eStorageImage; golWrite1.descriptorCount = 1; golWrite1.pImageInfo = &writeB;
    std::array<vk::WriteDescriptorSet,2> golWritesAB = { golWrite0, golWrite1 };
    deviceBundle.device.updateDescriptorSets( golWritesAB, {} );

    // GOL descriptor set 1: read B, write A
    vk::DescriptorImageInfo readB{}; readB.sampler = *sampler; readB.imageView = *stateBView; readB.imageLayout = vk::ImageLayout::eGeneral;
    vk::DescriptorImageInfo writeA{}; writeA.imageView = *stateAView; writeA.imageLayout = vk::ImageLayout::eGeneral;
    vk::WriteDescriptorSet golWrite2{}; golWrite2.dstSet = *golSets[1]; golWrite2.dstBinding = 0; golWrite2.descriptorType = vk::DescriptorType::eCombinedImageSampler; golWrite2.descriptorCount = 1; golWrite2.pImageInfo = &readB;
    vk::WriteDescriptorSet golWrite3{}; golWrite3.dstSet = *golSets[1]; golWrite3.dstBinding = 1; golWrite3.descriptorType = vk::DescriptorType::eStorageImage; golWrite3.descriptorCount = 1; golWrite3.pImageInfo = &writeA;
    std::array<vk::WriteDescriptorSet,2> golWritesBA = { golWrite2, golWrite3 };
    deviceBundle.device.updateDescriptorSets( golWritesBA, {} );

    // Graphics descriptor sets: sample A and sample B
    vk::WriteDescriptorSet gfxWriteA{}; gfxWriteA.dstSet = *gfxSets[0]; gfxWriteA.dstBinding = 0; gfxWriteA.descriptorType = vk::DescriptorType::eCombinedImageSampler; gfxWriteA.descriptorCount = 1; gfxWriteA.pImageInfo = &readA;
    vk::WriteDescriptorSet gfxWriteB{}; gfxWriteB.dstSet = *gfxSets[1]; gfxWriteB.dstBinding = 0; gfxWriteB.descriptorType = vk::DescriptorType::eCombinedImageSampler; gfxWriteB.descriptorCount = 1; gfxWriteB.pImageInfo = &readB;
    std::array<vk::WriteDescriptorSet,2> gfxWrites = { gfxWriteA, gfxWriteB };
    deviceBundle.device.updateDescriptorSets( gfxWrites, {} );

    // ImGui: create descriptor pool
    std::array<vk::DescriptorPoolSize, 11> imguiPoolSizes = {
      vk::DescriptorPoolSize{ vk::DescriptorType::eSampler, 1000 },
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

    // ImGui: initialize context and backends
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan( Display.window, true );

    ImGui_ImplVulkan_InitInfo imguiInitInfo{};
    imguiInitInfo.Instance       = *instance;
    imguiInitInfo.PhysicalDevice = *physicalDevice;
    imguiInitInfo.Device         = *deviceBundle.device;
    imguiInitInfo.QueueFamily    = deviceBundle.indices.graphicsFamily;
    imguiInitInfo.Queue          = *deviceBundle.graphicsQueue;
    imguiInitInfo.DescriptorPool = *imguiDescriptorPool;
    imguiInitInfo.RenderPass     = *renderPass;
    imguiInitInfo.MinImageCount  = static_cast<uint32_t>( framebuffers.size() );
    imguiInitInfo.ImageCount     = static_cast<uint32_t>( framebuffers.size() );
    imguiInitInfo.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;
    imguiInitInfo.UseDynamicRendering = false;
    ImGui_ImplVulkan_Init( &imguiInitInfo );

    // ImGui: recent backends build fonts automatically during NewFrame; no manual upload required

    // Create synchronization objects for 2 frames in flight
    core::SyncObjects syncObjects = core::createSyncObjects( deviceBundle.device, 2 );

    // Main render loop
    size_t currentFrame = 0;
    bool resize = false;
    bool seeded = false;
    bool useAB = true; // true: A->B then display B; false: B->A then display A
    while ( !glfwWindowShouldClose( Display.window ) )
    {
      glfwPollEvents();

      // Check if window was resized/minimized
      int width, height;
      glfwGetFramebufferSize( Display.window, &width, &height );
      if ( width == 0 || height == 0 )
      {
        glfwWaitEvents();
        continue;
      }

      if ( width != Display.extent.width || height != Display.extent.height )
      {
        // Update extent and recreate swapchain
        Display.extent = vk::Extent2D( static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) );
        deviceBundle.device.waitIdle();

        swapchain = core::createSwapchain( physicalDevice, deviceBundle.device, Display.surface, Display.extent, indices, &swapchain.swapchain );

        // Recreate framebuffers with new swapchain
        framebuffers = core::createFramebuffers( deviceBundle.device, renderPass, Display.extent, swapchain.imageViews );

        // Recreate command resources with new framebuffer count
        commandResources = core::createCommandResources( deviceBundle.device, deviceBundle.indices.graphicsFamily, framebuffers.size() );

        // Re-record commands for new framebuffers (pipeline uses dynamic viewport/scissor)
        core::recordTriangleCommands( commandResources.buffers, renderPass, framebuffers.data(), framebuffers.size(), Display.extent, graphicsPipeline );

        // Reset current frame to avoid issues with sync objects
        currentFrame = 0;
        resize = false;
      }

      // Start ImGui frame and show FPS window
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      if ( ImGui::Begin( "Stats" ) )
      {
        ImGui::Text( "FPS: %.1f", ImGui::GetIO().Framerate );
        ImGui::Button("adad");
      }
      ImGui::End();
      ImGui::Render();

      // Draw one frame
      try
      {
        (void)deviceBundle.device.waitForFences( *syncObjects.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX );

        uint32_t imageIndex = 0;
        auto     acq        = swapchain.swapchain.acquireNextImage( UINT64_MAX, *syncObjects.imageAvailable[currentFrame], nullptr );
        if ( acq.first != vk::Result::eSuccess && acq.first != vk::Result::eSuboptimalKHR )
        {
          throw std::runtime_error( "Failed to acquire swapchain image" );
        }
        imageIndex = acq.second;

        deviceBundle.device.resetFences( *syncObjects.inFlightFences[currentFrame] );

        auto & cb = commandResources.buffers[imageIndex];
        cb.reset();
        cb.begin( vk::CommandBufferBeginInfo{} );

        auto barrier = [&](vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags srcAccess, vk::AccessFlags dstAccess, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage){
          vk::ImageMemoryBarrier b{};
          b.oldLayout = oldLayout; b.newLayout = newLayout;
          b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          b.image = image;
          b.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
          b.subresourceRange.baseMipLevel = 0; b.subresourceRange.levelCount = 1;
          b.subresourceRange.baseArrayLayer = 0; b.subresourceRange.layerCount = 1;
          b.srcAccessMask = srcAccess; b.dstAccessMask = dstAccess;
          cb.pipelineBarrier( srcStage, dstStage, {}, nullptr, nullptr, b );
        };
        // Transition state images to GENERAL once
        barrier( *stateA, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {}, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader );
        barrier( *stateB, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {}, vk::AccessFlagBits::eShaderWrite, vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader );

        // Seed once
        if ( !seeded ) {
          cb.bindPipeline( vk::PipelineBindPoint::eCompute, *seedPipeline );
          // seed writes to A
          vk::DescriptorImageInfo seedDst{}; seedDst.imageView = *stateAView; seedDst.imageLayout = vk::ImageLayout::eGeneral;
          vk::WriteDescriptorSet seedWrite{}; seedWrite.dstSet = *golSets[1]; seedWrite.dstBinding = 1; seedWrite.descriptorType = vk::DescriptorType::eStorageImage; seedWrite.descriptorCount = 1; seedWrite.pImageInfo = &seedDst;
          deviceBundle.device.updateDescriptorSets( seedWrite, nullptr );
          cb.bindDescriptorSets( vk::PipelineBindPoint::eCompute, *golPipelineLayout, 0, *golSets[1], nullptr );
          cb.dispatch( (simExtent.width + 15)/16, (simExtent.height + 15)/16, 1 );
          seeded = true;
        }

        // Game of Life update (ping-pong)
        cb.bindPipeline( vk::PipelineBindPoint::eCompute, *golPipeline );
        if ( useAB ) {
          cb.bindDescriptorSets( vk::PipelineBindPoint::eCompute, *golPipelineLayout, 0, *golSets[0], nullptr );
        } else {
          cb.bindDescriptorSets( vk::PipelineBindPoint::eCompute, *golPipelineLayout, 0, *golSets[1], nullptr );
        }
        cb.dispatch( (simExtent.width + 15)/16, (simExtent.height + 15)/16, 1 );

        // Ensure written state is visible for sampling by fragment shader
        vk::Image writtenImage = useAB ? *stateB : *stateA;
        barrier( writtenImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eGeneral, vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eFragmentShader );

        // Begin render pass and draw fullscreen quad sampling the written state
        vk::ClearValue          clearColor = vk::ClearColorValue{ std::array<float, 4>{ 0.02f, 0.02f, 0.03f, 1.0f } };
        vk::RenderPassBeginInfo rpBegin{};
        rpBegin.renderPass        = *renderPass;
        rpBegin.framebuffer       = *framebuffers[imageIndex];
        rpBegin.renderArea.offset = vk::Offset2D{ 0, 0 };
        rpBegin.renderArea.extent = swapchain.extent;
        rpBegin.clearValueCount   = 1;
        rpBegin.pClearValues      = &clearColor;

        cb.beginRenderPass( rpBegin, vk::SubpassContents::eInline );
        cb.bindPipeline( vk::PipelineBindPoint::eGraphics, *graphicsPipeline );
        // Dynamic viewport/scissor are set in recordTriangleCommands for initial recording; set again
        vk::Viewport vp{}; vp.x=0; vp.y=0; vp.width = static_cast<float>(swapchain.extent.width); vp.height = static_cast<float>(swapchain.extent.height); vp.minDepth=0; vp.maxDepth=1;
        std::array<vk::Viewport,1> vps{ vp };
        cb.setViewport(0, vps);
        vk::Rect2D sc{}; sc.offset = vk::Offset2D{0,0}; sc.extent = swapchain.extent;
        std::array<vk::Rect2D,1> scs{ sc };
        cb.setScissor(0, scs);
        // Bind appropriate graphics descriptor set (sample written state)
        if ( useAB ) {
          cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *gfxSets[1], nullptr ); // sample B
        } else {
          cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *gfxSets[0], nullptr ); // sample A
        }
        cb.draw( 4, 1, 0, 0 ); // triangle strip fullscreen quad
        cb.endRenderPass();
        cb.end();

        useAB = !useAB;

        vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo         submitInfo{};
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &*syncObjects.imageAvailable[currentFrame];
        submitInfo.pWaitDstStageMask    = &waitStages;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &*cb;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &*syncObjects.renderFinished[currentFrame];

        deviceBundle.graphicsQueue.submit( submitInfo, *syncObjects.inFlightFences[currentFrame] );

        vk::PresentInfoKHR presentInfo{};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &*syncObjects.renderFinished[currentFrame];
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &*swapchain.swapchain;
        presentInfo.pImageIndices      = &imageIndex;
        (void)deviceBundle.presentQueue.presentKHR( presentInfo );

        currentFrame = ( currentFrame + 1 ) % syncObjects.imageAvailable.size();
      }
      catch ( std::exception const & err )
      {
        isDebug( std::println( "drawFrame exception: {}", err.what() ) );
        resize = true;
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
