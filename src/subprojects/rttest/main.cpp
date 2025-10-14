#include "bootstrap.hpp"
#include <array>

constexpr std::string_view AppName    = "RTTest";
constexpr std::string_view EngineName = "MyEngine";

// Simple triangle geometry
struct Vertex {
  glm::vec3 position;
};

// Camera matrices for ray generation
struct CameraData {
  glm::mat4 viewInverse;
  glm::mat4 projInverse;
};

// Helper to align buffer sizes
static uint32_t alignedSize(uint32_t value, uint32_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

// Helper to find memory type
static uint32_t findMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
  vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
  
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  
  throw std::runtime_error("Failed to find suitable memory type!");
}

// Helper to get buffer device address
static vk::DeviceAddress getBufferDeviceAddress(const vk::raii::Device& device, const vk::raii::Buffer& buffer) {
  vk::BufferDeviceAddressInfo info{};
  info.setBuffer(*buffer);
  return device.getBufferAddress(info);
}

// Create a buffer with memory
struct BufferBundle {
  vk::raii::Buffer buffer;
  vk::raii::DeviceMemory memory;
  vk::DeviceAddress deviceAddress = 0;
};

static BufferBundle createBuffer(
  const vk::raii::PhysicalDevice& physicalDevice,
  const vk::raii::Device& device,
  vk::DeviceSize size,
  vk::BufferUsageFlags usage,
  vk::MemoryPropertyFlags properties)
{
  BufferBundle bundle{nullptr, nullptr};
  
  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.setSize(size)
    .setUsage(usage)
    .setSharingMode(vk::SharingMode::eExclusive);
  
  bundle.buffer = vk::raii::Buffer{device, bufferInfo};
  
  vk::MemoryRequirements memReqs = bundle.buffer.getMemoryRequirements();
  
  vk::MemoryAllocateFlagsInfo allocFlags{};
  if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
    allocFlags.setFlags(vk::MemoryAllocateFlagBits::eDeviceAddress);
  }
  
  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.setAllocationSize(memReqs.size)
    .setMemoryTypeIndex(findMemoryType(physicalDevice, memReqs.memoryTypeBits, properties))
    .setPNext(&allocFlags);
  
  bundle.memory = vk::raii::DeviceMemory{device, allocInfo};
  bundle.buffer.bindMemory(*bundle.memory, 0);
  
  if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
    bundle.deviceAddress = getBufferDeviceAddress(device, bundle.buffer);
  }
  
  return bundle;
}

int main()
{
  isDebug( std::println( "LOADING UP RTTEST EXAMPLE!\n" ); );
  try
  {
    vk::raii::Context context;
    
    vk::raii::Instance instance = core::createInstance( context, std::string( AppName ), std::string( EngineName ) );
    
    vk::raii::PhysicalDevices physicalDevices( instance );
    vk::raii::PhysicalDevice physicalDevice = core::selectPhysicalDevice( physicalDevices );
    
    // Check ray tracing support
    auto rtProps = physicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    auto rtPipelineProps = rtProps.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    
    isDebug( std::println( "Ray Tracing supported!" ); );
    isDebug( std::println( "  Shader Group Handle Size: {}", rtPipelineProps.shaderGroupHandleSize ); );
    
    core::DisplayBundle displayBundle( instance, "Ray Tracing Test", vk::Extent2D( 1280, 720 ) );
    
    core::QueueFamilyIndices queueFamilyIndices = core::findQueueFamilies( physicalDevice, displayBundle.surface );
    
    core::DeviceBundle deviceBundle = core::createDeviceWithQueues( physicalDevice, queueFamilyIndices );
    
    core::SwapchainBundle swapchainBundle = core::createSwapchain( physicalDevice, deviceBundle.device, displayBundle.surface, displayBundle.extent, queueFamilyIndices );
    
    // ===== Create simple triangle geometry =====
    std::vector<Vertex> vertices = {
      {{0.0f, -0.5f, -2.0f}},
      {{0.5f, 0.5f, -2.0f}},
      {{-0.5f, 0.5f, -2.0f}}
    };
    
    std::vector<uint32_t> indices = {0, 1, 2};
    
    // Create vertex buffer
    vk::DeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
    auto vertexBuffer = createBuffer(
      physicalDevice, deviceBundle.device,
      vertexBufferSize,
      vk::BufferUsageFlagBits::eVertexBuffer |
      vk::BufferUsageFlagBits::eShaderDeviceAddress |
      vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    
    // Upload vertex data
    void* vertexData = vertexBuffer.memory.mapMemory(0, vertexBufferSize);
    memcpy(vertexData, vertices.data(), vertexBufferSize);
    vertexBuffer.memory.unmapMemory();
    
    // Create index buffer
    vk::DeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
    auto indexBuffer = createBuffer(
      physicalDevice, deviceBundle.device,
      indexBufferSize,
      vk::BufferUsageFlagBits::eIndexBuffer |
      vk::BufferUsageFlagBits::eShaderDeviceAddress |
      vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    
    // Upload index data
    void* indexData = indexBuffer.memory.mapMemory(0, indexBufferSize);
    memcpy(indexData, indices.data(), indexBufferSize);
    indexBuffer.memory.unmapMemory();
    
    // ===== Build Bottom-Level Acceleration Structure (BLAS) =====
    vk::AccelerationStructureGeometryKHR geometry{};
    geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles)
      .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
    
    geometry.geometry.triangles
      .setVertexFormat(vk::Format::eR32G32B32Sfloat)
      .setVertexData(vertexBuffer.deviceAddress)
      .setVertexStride(sizeof(Vertex))
      .setMaxVertex(static_cast<uint32_t>(vertices.size() - 1))
      .setIndexType(vk::IndexType::eUint32)
      .setIndexData(indexBuffer.deviceAddress);
    
    vk::AccelerationStructureBuildGeometryInfoKHR blasBuildInfo{};
    blasBuildInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
      .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild)
      .setGeometries(geometry);
    
    uint32_t primitiveCount = static_cast<uint32_t>(indices.size() / 3);
    
    vk::AccelerationStructureBuildSizesInfoKHR blasSizeInfo = 
      deviceBundle.device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        blasBuildInfo,
        primitiveCount
      );
    
    // Create BLAS buffer
    auto blasBuffer = createBuffer(
      physicalDevice, deviceBundle.device,
      blasSizeInfo.accelerationStructureSize,
      vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    
    // Create BLAS
    vk::AccelerationStructureCreateInfoKHR blasCreateInfo{};
    blasCreateInfo.setBuffer(*blasBuffer.buffer)
      .setSize(blasSizeInfo.accelerationStructureSize)
      .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    
    vk::raii::AccelerationStructureKHR blas{deviceBundle.device, blasCreateInfo};
    
    // Create scratch buffer for building
    auto blasScratchBuffer = createBuffer(
      physicalDevice, deviceBundle.device,
      blasSizeInfo.buildScratchSize,
      vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    
    // Build BLAS
    vk::CommandPoolCreateInfo cmdPoolInfo{vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndices.graphicsFamily.value()};
    vk::raii::CommandPool commandPool{deviceBundle.device, cmdPoolInfo};
    
    vk::CommandBufferAllocateInfo cmdAllocInfo{*commandPool, vk::CommandBufferLevel::ePrimary, 1};
    vk::raii::CommandBuffers buildCmds{deviceBundle.device, cmdAllocInfo};
    auto& buildCmd = buildCmds[0];
    
    buildCmd.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    
    blasBuildInfo.setDstAccelerationStructure(*blas)
      .setScratchData(blasScratchBuffer.deviceAddress);
    
    vk::AccelerationStructureBuildRangeInfoKHR blasBuildRange{};
    blasBuildRange.setPrimitiveCount(primitiveCount);
    
    buildCmd.buildAccelerationStructuresKHR(blasBuildInfo, &blasBuildRange);
    
    buildCmd.end();
    
    // Submit and wait
    vk::SubmitInfo submitInfo{};
    submitInfo.setCommandBuffers(*buildCmd);
    deviceBundle.graphicsQueue.submit(submitInfo);
    deviceBundle.graphicsQueue.waitIdle();
    
    isDebug( std::println( "BLAS built successfully!" ); );
    
    // Get BLAS device address
    vk::AccelerationStructureDeviceAddressInfoKHR blasAddressInfo{};
    blasAddressInfo.setAccelerationStructure(*blas);
    vk::DeviceAddress blasAddress = deviceBundle.device.getAccelerationStructureAddressKHR(blasAddressInfo);
    
    // ===== Build Top-Level Acceleration Structure (TLAS) =====
    vk::TransformMatrixKHR transformMatrix{};
    transformMatrix.matrix[0][0] = 1.0f;
    transformMatrix.matrix[1][1] = 1.0f;
    transformMatrix.matrix[2][2] = 1.0f;
    
    vk::AccelerationStructureInstanceKHR asInstance{};
    asInstance.setTransform(transformMatrix)
      .setInstanceCustomIndex(0)  // Material ID
      .setMask(0xFF)
      .setInstanceShaderBindingTableRecordOffset(0)
      .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
      .setAccelerationStructureReference(blasAddress);
    
    // Create instance buffer
    auto instanceBuffer = createBuffer(
      physicalDevice, deviceBundle.device,
      sizeof(vk::AccelerationStructureInstanceKHR),
      vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    
    void* instanceData = instanceBuffer.memory.mapMemory(0, sizeof(vk::AccelerationStructureInstanceKHR));
    memcpy(instanceData, &asInstance, sizeof(vk::AccelerationStructureInstanceKHR));
    instanceBuffer.memory.unmapMemory();
    
    vk::AccelerationStructureGeometryKHR tlasGeometry{};
    tlasGeometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    tlasGeometry.geometry.instances.setArrayOfPointers(VK_FALSE)
      .setData(instanceBuffer.deviceAddress);
    
    vk::AccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{};
    tlasBuildInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
      .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
      .setGeometries(tlasGeometry);
    
    uint32_t instanceCount = 1;
    
    vk::AccelerationStructureBuildSizesInfoKHR tlasSizeInfo = 
      deviceBundle.device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        tlasBuildInfo,
        instanceCount
      );
    
    auto tlasBuffer = createBuffer(
      physicalDevice, deviceBundle.device,
      tlasSizeInfo.accelerationStructureSize,
      vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    
    vk::AccelerationStructureCreateInfoKHR tlasCreateInfo{};
    tlasCreateInfo.setBuffer(*tlasBuffer.buffer)
      .setSize(tlasSizeInfo.accelerationStructureSize)
      .setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    
    vk::raii::AccelerationStructureKHR tlas{deviceBundle.device, tlasCreateInfo};
    
    auto tlasScratchBuffer = createBuffer(
      physicalDevice, deviceBundle.device,
      tlasSizeInfo.buildScratchSize,
      vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    
    // Build TLAS
    buildCmd.reset();
    buildCmd.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    
    tlasBuildInfo.setDstAccelerationStructure(*tlas)
      .setScratchData(tlasScratchBuffer.deviceAddress);
    
    vk::AccelerationStructureBuildRangeInfoKHR tlasBuildRange{};
    tlasBuildRange.setPrimitiveCount(instanceCount);
    
    buildCmd.buildAccelerationStructuresKHR(tlasBuildInfo, &tlasBuildRange);
    buildCmd.end();
    
    submitInfo.setCommandBuffers(*buildCmd);
    deviceBundle.graphicsQueue.submit(submitInfo);
    deviceBundle.graphicsQueue.waitIdle();
    
    isDebug( std::println( "TLAS built successfully!" ); );
    
    // ===== Create ray tracing output image =====
    vk::ImageCreateInfo imageInfo{};
    imageInfo.setImageType(vk::ImageType::e2D)
      .setFormat(vk::Format::eR8G8B8A8Unorm)
      .setExtent(vk::Extent3D{swapchainBundle.extent.width, swapchainBundle.extent.height, 1})
      .setMipLevels(1)
      .setArrayLayers(1)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setTiling(vk::ImageTiling::eOptimal)
      .setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc)
      .setSharingMode(vk::SharingMode::eExclusive)
      .setInitialLayout(vk::ImageLayout::eUndefined);
    
    vk::raii::Image outputImage{deviceBundle.device, imageInfo};
    
    vk::MemoryRequirements imageMemReqs = outputImage.getMemoryRequirements();
    
    vk::MemoryAllocateInfo imageAllocInfo{};
    imageAllocInfo.setAllocationSize(imageMemReqs.size)
      .setMemoryTypeIndex(findMemoryType(physicalDevice, imageMemReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    
    vk::raii::DeviceMemory outputImageMemory{deviceBundle.device, imageAllocInfo};
    outputImage.bindMemory(*outputImageMemory, 0);
    
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.setImage(*outputImage)
      .setViewType(vk::ImageViewType::e2D)
      .setFormat(vk::Format::eR8G8B8A8Unorm)
      .setSubresourceRange(vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    
    vk::raii::ImageView outputImageView{deviceBundle.device, viewInfo};
    
    // ===== Create material buffer =====
    std::vector<glm::vec4> materials = {
      glm::vec4(1.0f, 0.3f, 0.3f, 0.0f)  // Red triangle
    };
    
    auto materialBuffer = createBuffer(
      physicalDevice, deviceBundle.device,
      sizeof(glm::vec4) * materials.size(),
      vk::BufferUsageFlagBits::eStorageBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    
    void* materialData = materialBuffer.memory.mapMemory(0, sizeof(glm::vec4) * materials.size());
    memcpy(materialData, materials.data(), sizeof(glm::vec4) * materials.size());
    materialBuffer.memory.unmapMemory();
    
    // ===== Create camera uniform buffer =====
    auto cameraBuffer = createBuffer(
      physicalDevice, deviceBundle.device,
      sizeof(CameraData),
      vk::BufferUsageFlagBits::eUniformBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    
    void* cameraData = cameraBuffer.memory.mapMemory(0, sizeof(CameraData));
    
    // ===== Create descriptor sets =====
    std::array<vk::DescriptorSetLayoutBinding, 3> set0Bindings{};
    // Binding 0: TLAS
    set0Bindings[0].setBinding(0)
      .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
      .setDescriptorCount(1)
      .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);
    
    // Binding 1: Output image
    set0Bindings[1].setBinding(1)
      .setDescriptorType(vk::DescriptorType::eStorageImage)
      .setDescriptorCount(1)
      .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);
    
    // Binding 2: Camera uniform
    set0Bindings[2].setBinding(2)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(1)
      .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);
    
    vk::DescriptorSetLayoutCreateInfo set0LayoutInfo{};
    set0LayoutInfo.setBindings(set0Bindings);
    
    vk::raii::DescriptorSetLayout descriptorSetLayout0{deviceBundle.device, set0LayoutInfo};
    
    std::array<vk::DescriptorSetLayoutBinding, 1> set1Bindings{};
    // Binding 0: Materials buffer
    set1Bindings[0].setBinding(0)
      .setDescriptorType(vk::DescriptorType::eStorageBuffer)
      .setDescriptorCount(1)
      .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
    
    vk::DescriptorSetLayoutCreateInfo set1LayoutInfo{};
    set1LayoutInfo.setBindings(set1Bindings);
    
    vk::raii::DescriptorSetLayout descriptorSetLayout1{deviceBundle.device, set1LayoutInfo};
    
    std::array<vk::DescriptorPoolSize, 4> poolSizes{};
    poolSizes[0].setType(vk::DescriptorType::eAccelerationStructureKHR).setDescriptorCount(1);
    poolSizes[1].setType(vk::DescriptorType::eStorageImage).setDescriptorCount(1);
    poolSizes[2].setType(vk::DescriptorType::eUniformBuffer).setDescriptorCount(1);
    poolSizes[3].setType(vk::DescriptorType::eStorageBuffer).setDescriptorCount(1);
    
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.setMaxSets(2)
      .setPoolSizes(poolSizes);
    
    vk::raii::DescriptorPool descriptorPool{deviceBundle.device, poolInfo};
    
    std::array<vk::DescriptorSetLayout, 2> layouts = {*descriptorSetLayout0, *descriptorSetLayout1};
    
    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.setDescriptorPool(*descriptorPool)
      .setSetLayouts(layouts);
    
    vk::raii::DescriptorSets descriptorSets{deviceBundle.device, allocInfo};
    
    // Update descriptor set 0
    vk::WriteDescriptorSetAccelerationStructureKHR writeAS{};
    writeAS.setAccelerationStructures(*tlas);
    
    vk::WriteDescriptorSet writeSet0_0{};
    writeSet0_0.setDstSet(*descriptorSets[0])
      .setDstBinding(0)
      .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
      .setDescriptorCount(1)
      .setPNext(&writeAS);
    
    vk::DescriptorImageInfo imageDescInfo{};
    imageDescInfo.setImageView(*outputImageView)
      .setImageLayout(vk::ImageLayout::eGeneral);
    
    vk::WriteDescriptorSet writeSet0_1{};
    writeSet0_1.setDstSet(*descriptorSets[0])
      .setDstBinding(1)
      .setDescriptorType(vk::DescriptorType::eStorageImage)
      .setImageInfo(imageDescInfo);
    
    vk::DescriptorBufferInfo cameraBufferInfo{};
    cameraBufferInfo.setBuffer(*cameraBuffer.buffer)
      .setRange(VK_WHOLE_SIZE);
    
    vk::WriteDescriptorSet writeSet0_2{};
    writeSet0_2.setDstSet(*descriptorSets[0])
      .setDstBinding(2)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setBufferInfo(cameraBufferInfo);
    
    // Update descriptor set 1
    vk::DescriptorBufferInfo materialBufferInfo{};
    materialBufferInfo.setBuffer(*materialBuffer.buffer)
      .setRange(VK_WHOLE_SIZE);
    
    vk::WriteDescriptorSet writeSet1_0{};
    writeSet1_0.setDstSet(*descriptorSets[1])
      .setDstBinding(0)
      .setDescriptorType(vk::DescriptorType::eStorageBuffer)
      .setBufferInfo(materialBufferInfo);
    
    std::array<vk::WriteDescriptorSet, 4> writes = {writeSet0_0, writeSet0_1, writeSet0_2, writeSet1_0};
    deviceBundle.device.updateDescriptorSets(writes, {});
    
    // ===== Load shaders =====
    std::vector<uint32_t> raygenCode = core::readSpirvFile("shaders/raygen.rgen.spv");
    std::vector<uint32_t> closesthitCode = core::readSpirvFile("shaders/closesthit.rchit.spv");
    std::vector<uint32_t> missCode = core::readSpirvFile("shaders/miss.rmiss.spv");
    
    vk::raii::ShaderModule raygenModule = core::createShaderModule(deviceBundle.device, raygenCode);
    vk::raii::ShaderModule closesthitModule = core::createShaderModule(deviceBundle.device, closesthitCode);
    vk::raii::ShaderModule missModule = core::createShaderModule(deviceBundle.device, missCode);
    
    // ===== Create ray tracing pipeline =====
    std::array<vk::PipelineShaderStageCreateInfo, 3> shaderStages{};
    shaderStages[0].setStage(vk::ShaderStageFlagBits::eRaygenKHR)
      .setModule(*raygenModule)
      .setPName("main");
    
    shaderStages[1].setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
      .setModule(*closesthitModule)
      .setPName("main");
    
    shaderStages[2].setStage(vk::ShaderStageFlagBits::eMissKHR)
      .setModule(*missModule)
      .setPName("main");
    
    std::array<vk::RayTracingShaderGroupCreateInfoKHR, 3> shaderGroups{};
    // Group 0: Raygen
    shaderGroups[0].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
      .setGeneralShader(0)
      .setClosestHitShader(VK_SHADER_UNUSED_KHR)
      .setAnyHitShader(VK_SHADER_UNUSED_KHR)
      .setIntersectionShader(VK_SHADER_UNUSED_KHR);
    
    // Group 1: Hit group
    shaderGroups[1].setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
      .setGeneralShader(VK_SHADER_UNUSED_KHR)
      .setClosestHitShader(1)
      .setAnyHitShader(VK_SHADER_UNUSED_KHR)
      .setIntersectionShader(VK_SHADER_UNUSED_KHR);
    
    // Group 2: Miss
    shaderGroups[2].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
      .setGeneralShader(2)
      .setClosestHitShader(VK_SHADER_UNUSED_KHR)
      .setAnyHitShader(VK_SHADER_UNUSED_KHR)
      .setIntersectionShader(VK_SHADER_UNUSED_KHR);
    
    std::array<vk::DescriptorSetLayout, 2> pipelineLayouts = {*descriptorSetLayout0, *descriptorSetLayout1};
    
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setSetLayouts(pipelineLayouts);
    
    vk::raii::PipelineLayout pipelineLayout{deviceBundle.device, pipelineLayoutInfo};
    
    vk::RayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.setStages(shaderStages)
      .setGroups(shaderGroups)
      .setMaxPipelineRayRecursionDepth(1)
      .setLayout(*pipelineLayout);
    
    auto rtPipelines = vk::raii::Pipelines{deviceBundle.device, nullptr, nullptr, pipelineInfo};
    vk::raii::Pipeline rtPipeline = std::move(rtPipelines[0]);
    
    isDebug( std::println( "Ray tracing pipeline created!" ); );
    
    // ===== Create Shader Binding Table (SBT) =====
    uint32_t handleSize = rtPipelineProps.shaderGroupHandleSize;
    uint32_t handleAlignment = rtPipelineProps.shaderGroupHandleAlignment;
    uint32_t alignedHandleSize = alignedSize(handleSize, handleAlignment);
    
    uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
    uint32_t sbtSize = groupCount * alignedHandleSize;
    
    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    vk::Result result = static_cast<vk::Device>(deviceBundle.device).getRayTracingShaderGroupHandlesKHR(*rtPipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());
    
    auto sbtBuffer = createBuffer(
      physicalDevice, deviceBundle.device,
      sbtSize,
      vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    
    void* sbtData = sbtBuffer.memory.mapMemory(0, sbtSize);
    for (uint32_t i = 0; i < groupCount; i++) {
      memcpy(static_cast<uint8_t*>(sbtData) + i * alignedHandleSize, 
             shaderHandleStorage.data() + i * handleSize, 
             handleSize);
    }
    sbtBuffer.memory.unmapMemory();
    
    vk::DeviceAddress sbtAddress = sbtBuffer.deviceAddress;
    
    vk::StridedDeviceAddressRegionKHR raygenRegion{};
    raygenRegion.setDeviceAddress(sbtAddress)
      .setStride(alignedHandleSize)
      .setSize(alignedHandleSize);
    
    vk::StridedDeviceAddressRegionKHR hitRegion{};
    hitRegion.setDeviceAddress(sbtAddress + alignedHandleSize)
      .setStride(alignedHandleSize)
      .setSize(alignedHandleSize);
    
    vk::StridedDeviceAddressRegionKHR missRegion{};
    missRegion.setDeviceAddress(sbtAddress + 2 * alignedHandleSize)
      .setStride(alignedHandleSize)
      .setSize(alignedHandleSize);
    
    vk::StridedDeviceAddressRegionKHR callableRegion{};
    
    // ===== Create command buffers for rendering =====
    constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
    
    vk::CommandBufferAllocateInfo cmdInfo{*commandPool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT};
    vk::raii::CommandBuffers cmds{deviceBundle.device, cmdInfo};
    
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;
    
    imageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      imageAvailableSemaphores.emplace_back(deviceBundle.device, vk::SemaphoreCreateInfo{});
      renderFinishedSemaphores.emplace_back(deviceBundle.device, vk::SemaphoreCreateInfo{});
      inFlightFences.emplace_back(deviceBundle.device, vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled});
    }
    
    size_t currentFrame = 0;
    
    isDebug( std::println( "Entering main loop...\n" ); );
    
    while ( !glfwWindowShouldClose( displayBundle.window ) )
    {
      glfwPollEvents();
      
      // Wait for fence
      (void)deviceBundle.device.waitForFences({*inFlightFences[currentFrame]}, VK_TRUE, UINT64_MAX);
      
      // Acquire image
      auto acquire = swapchainBundle.swapchain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[currentFrame], nullptr);
      uint32_t imageIndex = acquire.second;
      
      deviceBundle.device.resetFences({*inFlightFences[currentFrame]});
      
      // Update camera
      CameraData camera{};
      glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      glm::mat4 proj = glm::perspective(glm::radians(60.0f), 
                                       (float)swapchainBundle.extent.width / (float)swapchainBundle.extent.height,
                                       0.1f, 100.0f);
      proj[1][1] *= -1;  // Flip Y for Vulkan
      
      camera.viewInverse = glm::inverse(view);
      camera.projInverse = glm::inverse(proj);
      
      memcpy(cameraData, &camera, sizeof(CameraData));
      
      // Record command buffer
      auto& cmd = cmds[currentFrame];
      cmd.reset();
      cmd.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
      
      // Transition output image to general layout
      vk::ImageMemoryBarrier barrier{};
      barrier.setImage(*outputImage)
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eGeneral)
        .setSubresourceRange(vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
      
      cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        {},
        {}, {}, barrier
      );
      
      // Bind ray tracing pipeline and trace rays
      cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *rtPipeline);
      
      std::array<vk::DescriptorSet, 2> sets = {*descriptorSets[0], *descriptorSets[1]};
      cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *pipelineLayout, 0, sets, {});
      
      cmd.traceRaysKHR(
        raygenRegion,
        missRegion,
        hitRegion,
        callableRegion,
        swapchainBundle.extent.width,
        swapchainBundle.extent.height,
        1
      );
      
      // Transition output image for transfer
      barrier.setOldLayout(vk::ImageLayout::eGeneral)
        .setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
      
      cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        vk::PipelineStageFlagBits::eTransfer,
        {},
        {}, {}, barrier
      );
      
      // Transition swapchain image to transfer dst
      vk::ImageMemoryBarrier swapchainBarrier{};
      swapchainBarrier.setImage(swapchainBundle.images[imageIndex])
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
        .setSubresourceRange(vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
      
      cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer,
        {},
        {}, {}, swapchainBarrier
      );
      
      // Copy output image to swapchain
      vk::ImageCopy copyRegion{};
      copyRegion.setSrcSubresource(vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1})
        .setDstSubresource(vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1})
        .setExtent(vk::Extent3D{swapchainBundle.extent.width, swapchainBundle.extent.height, 1});
      
      cmd.copyImage(*outputImage, vk::ImageLayout::eTransferSrcOptimal,
                   swapchainBundle.images[imageIndex], vk::ImageLayout::eTransferDstOptimal,
                   copyRegion);
      
      // Transition swapchain image for present
      swapchainBarrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::ePresentSrcKHR);
      
      cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        {},
        {}, {}, swapchainBarrier
      );
      
      cmd.end();
      
      // Submit
      vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eRayTracingShaderKHR;
      
      vk::SubmitInfo submitInfo{};
      submitInfo.setWaitSemaphores(*imageAvailableSemaphores[currentFrame])
        .setWaitDstStageMask(waitStage)
        .setCommandBuffers(*cmd)
        .setSignalSemaphores(*renderFinishedSemaphores[currentFrame]);
      
      deviceBundle.graphicsQueue.submit(submitInfo, *inFlightFences[currentFrame]);
      
      // Present
      vk::PresentInfoKHR presentInfo{};
      presentInfo.setWaitSemaphores(*renderFinishedSemaphores[currentFrame])
        .setSwapchains(*swapchainBundle.swapchain)
        .setImageIndices(imageIndex);
      
      (void)deviceBundle.graphicsQueue.presentKHR(presentInfo);
      
      currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    
    deviceBundle.device.waitIdle();
    
    cameraBuffer.memory.unmapMemory();
    
    isDebug( std::println( "Shutting down..." ); );
  }
  catch ( vk::SystemError & err )
  {
    std::println( "vk::SystemError: {}", err.what() );
    exit( -1 );
  }
  catch ( std::exception & err )
  {
    std::println( "std::exception: {}", err.what() );
    exit( -1 );
  }
  catch ( ... )
  {
    std::println( "unknown error" );
    exit( -1 );
  }
  
  return 0;
}

