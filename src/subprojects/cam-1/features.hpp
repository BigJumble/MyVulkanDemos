#pragma once
#include "vulkan/vulkan.hpp"

#include <vector>
#include <vulkan/vulkan_raii.hpp>


// clang-format off
namespace cfg {

  // -------------------------------------------------------------------------
  // Build feature chain from bottom to top
  // Each feature links to the one declared above it via setPNext()
  // -------------------------------------------------------------------------
  

  inline vk::PhysicalDeviceRobustness2FeaturesKHR robustness2Features = 
      vk::PhysicalDeviceRobustness2FeaturesKHR()
          .setRobustBufferAccess2(true)
          .setRobustImageAccess2(true);

        //   .setPNext();

  // Vulkan Version Features (1.1, 1.2, 1.3, 1.4) - bottom of chain
  inline vk::PhysicalDeviceVulkan11Features vulkan11Features = 
      vk::PhysicalDeviceVulkan11Features()
          .setPNext(&robustness2Features);
      // .setShaderDrawParameters(true);
  
  inline vk::PhysicalDeviceVulkan12Features vulkan12Features = 
      vk::PhysicalDeviceVulkan12Features()
          .setBufferDeviceAddress(true)
          .setDescriptorIndexing(true)
          .setRuntimeDescriptorArray(true)
          .setDescriptorBindingPartiallyBound(true)
          .setTimelineSemaphore(true)
          .setVulkanMemoryModel(true)
          .setVulkanMemoryModelDeviceScope(true)
          .setScalarBlockLayout(true)
          .setStorageBuffer8BitAccess(true)
          .setPNext(&vulkan11Features);
  
  inline vk::PhysicalDeviceVulkan13Features vulkan13Features = 
      vk::PhysicalDeviceVulkan13Features()
          .setDynamicRendering(true)
          .setSynchronization2(true)
          .setMaintenance4(true)
          .setPNext(&vulkan12Features);
  
  inline vk::PhysicalDeviceVulkan14Features vulkan14Features = 
      vk::PhysicalDeviceVulkan14Features()
          .setMaintenance5(true)
          .setMaintenance6(true)
          .setPNext(&vulkan13Features);
  
  // Extension Features
  inline vk::PhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures = 
      vk::PhysicalDeviceShaderObjectFeaturesEXT()
          .setShaderObject(true)
          .setPNext(&vulkan14Features);
  
  inline vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3Features = 
      vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT()
          .setExtendedDynamicState3LineRasterizationMode(true)
          .setPNext(&shaderObjectFeatures);
  
  inline vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenance1Features = 
      vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT()
          .setSwapchainMaintenance1(true)
          .setPNext(&extendedDynamicState3Features);
  
  inline vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageableDeviceLocalMemoryFeatures = 
      vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT()
          .setPNext(&swapchainMaintenance1Features);
  
  inline vk::PhysicalDeviceMemoryPriorityFeaturesEXT memoryPriorityFeatures = 
      vk::PhysicalDeviceMemoryPriorityFeaturesEXT()
          .setPNext(&pageableDeviceLocalMemoryFeatures);
  
  inline vk::PhysicalDeviceMaintenance7FeaturesKHR maintenance7Features = 
      vk::PhysicalDeviceMaintenance7FeaturesKHR()
          .setMaintenance7(true)
          .setPNext(&memoryPriorityFeatures);
  
  inline vk::PhysicalDeviceMaintenance8FeaturesKHR maintenance8Features = 
      vk::PhysicalDeviceMaintenance8FeaturesKHR()
          .setMaintenance8(true)
          .setPNext(&maintenance7Features);
  
  inline vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = 
      vk::PhysicalDeviceAccelerationStructureFeaturesKHR()
          .setPNext(&maintenance8Features);
      // .setAccelerationStructure(true);
  
  inline vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = 
      vk::PhysicalDeviceRayTracingPipelineFeaturesKHR()
          .setPNext(&accelerationStructureFeatures);
      // .setRayTracingPipeline(true);
  
  inline vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = 
      vk::PhysicalDeviceRayQueryFeaturesKHR()
          .setPNext(&rayTracingPipelineFeatures);
      // .setRayQuery(true);
  
  // Core Vulkan Features - top of chain
  inline vk::PhysicalDeviceFeatures coreFeatures = 
      vk::PhysicalDeviceFeatures()
          .setSamplerAnisotropy(true)
          .setFillModeNonSolid(true)
          .setFragmentStoresAndAtomics(true)
          .setVertexPipelineStoresAndAtomics(true)
          .setShaderInt64(true)
          .setRobustBufferAccess(true)
          .setWideLines(true);
  
  inline vk::PhysicalDeviceFeatures2 enabledFeaturesChain = 
      vk::PhysicalDeviceFeatures2()
          .setFeatures(coreFeatures)
          .setPNext(&rayQueryFeatures);

  // clang-format on

  inline const std::vector<const char *> getRequiredExtensions = {
    // KHR extensions
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_MAINTENANCE_7_EXTENSION_NAME,
    VK_KHR_MAINTENANCE_8_EXTENSION_NAME,

    // EXT extensions
    VK_EXT_SHADER_OBJECT_EXTENSION_NAME,

    VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,

    VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,

    // Ray tracing extensions
    // VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    // VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    // VK_KHR_RAY_QUERY_EXTENSION_NAME,
    // VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,

    // Pageable device local memory extensions
    // VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME,
    // VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME,
  };

  inline const std::vector<const char *> InstanceExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
    VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
    VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_METAL_EXT )
    VK_EXT_METAL_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_VI_NN )
    VK_NN_VI_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_WAYLAND_KHR )
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_WIN32_KHR )
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_XCB_KHR )
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_XLIB_KHR )
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif defined( VK_USE_PLATFORM_XLIB_XRANDR_EXT )
    VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME,
#endif
  };

}  // namespace cfg
