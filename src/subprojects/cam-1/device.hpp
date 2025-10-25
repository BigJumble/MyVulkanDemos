#pragma once
#include <vector>
#include <vulkan/vulkan_raii.hpp>

// ============================================================================
// Device Feature Configuration for Subproject
// ============================================================================
// This header provides a simple way to configure Vulkan device features.
// To customize for your subproject:
//   1. Enable/disable features by setting them to true/false
//   2. Comment out feature structs in the chain if not needed
//   3. Add required extensions to the extensions list
// ============================================================================
namespace cfg
{
  struct EnabledFeatures
  {
    // -------------------------------------------------------------------------
    // Core Vulkan Features
    // -------------------------------------------------------------------------
    vk::PhysicalDeviceFeatures  features{};
    vk::PhysicalDeviceFeatures2 features2{};

    // -------------------------------------------------------------------------
    // Vulkan Version Features (1.1, 1.2, 1.3, 1.4)
    // -------------------------------------------------------------------------
    vk::PhysicalDeviceVulkan11Features vulkan11Features{};
    vk::PhysicalDeviceVulkan12Features vulkan12Features{};
    vk::PhysicalDeviceVulkan13Features vulkan13Features{};
    vk::PhysicalDeviceVulkan14Features vulkan14Features{};

    // -------------------------------------------------------------------------
    // Extension Features
    // -------------------------------------------------------------------------
    vk::PhysicalDeviceShaderObjectFeaturesEXT              shaderObjectFeatures{};
    vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT     extendedDynamicState3Features{};
    vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT     swapchainMaintenance1Features{};
    vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageableDeviceLocalMemoryFeatures{};
    vk::PhysicalDeviceMemoryPriorityFeaturesEXT            memoryPriorityFeatures{};
    vk::PhysicalDeviceMaintenance7FeaturesKHR              maintenance7Features{};
    vk::PhysicalDeviceMaintenance8FeaturesKHR              maintenance8Features{};
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR     accelerationStructureFeatures{};
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR        rayTracingPipelineFeatures{};
    vk::PhysicalDeviceRayQueryFeaturesKHR                  rayQueryFeatures{};

    EnabledFeatures()
    {
      // --- Core Features (Vulkan 1.0) ---
      features.setSamplerAnisotropy( true );
      features.setFillModeNonSolid( true );
      features.setWideLines( true );

      features2.setFeatures( features );

      // --- Vulkan 1.1 Features ---
      // vulkan11Features.setShaderDrawParameters( true );

      // --- Vulkan 1.2 Features ---
      vulkan12Features.setBufferDeviceAddress( true );
      vulkan12Features.setDescriptorIndexing( true );
      vulkan12Features.setRuntimeDescriptorArray( true );
      vulkan12Features.setDescriptorBindingPartiallyBound( true );
      vulkan12Features.setTimelineSemaphore( true );

      // --- Vulkan 1.3 Features ---
      vulkan13Features.setDynamicRendering( true );
      vulkan13Features.setSynchronization2( true );
      vulkan13Features.setMaintenance4( true );

      // --- Vulkan 1.4 Features ---
      vulkan14Features.setMaintenance5( true );
      vulkan14Features.setMaintenance6( true );

      // --- Shader Object Extension ---
      shaderObjectFeatures.setShaderObject( true );

      // --- Maintenance Extensions ---
      maintenance7Features.setMaintenance7( true );
      maintenance8Features.setMaintenance8( true );

      // swapchain maintenance 1 features
      swapchainMaintenance1Features.setSwapchainMaintenance1( true );

      // --- Ray Tracing Features ---
    //   accelerationStructureFeatures.setAccelerationStructure( true );
    //   rayTracingPipelineFeatures.setRayTracingPipeline( true );
    //   rayQueryFeatures.setRayQuery( true );
    }

    const void * makeFeatureChain()
    {
      // Build the pNext chain (reverse order: last feature first)
      void * chainTail = nullptr;

      // Comment out any of these if not needed
      rayQueryFeatures.pNext                  = chainTail;
      chainTail                               = &rayQueryFeatures;
      rayTracingPipelineFeatures.pNext        = chainTail;
      chainTail                               = &rayTracingPipelineFeatures;
      accelerationStructureFeatures.pNext     = chainTail;
      chainTail                               = &accelerationStructureFeatures;
      extendedDynamicState3Features.pNext     = chainTail;
      chainTail                               = &extendedDynamicState3Features;
      swapchainMaintenance1Features.pNext     = chainTail;
      chainTail                               = &swapchainMaintenance1Features;
      pageableDeviceLocalMemoryFeatures.pNext = chainTail;
      chainTail                               = &pageableDeviceLocalMemoryFeatures;
      memoryPriorityFeatures.pNext            = chainTail;
      chainTail                               = &memoryPriorityFeatures;
      maintenance8Features.pNext              = chainTail;
      chainTail                               = &maintenance8Features;
      maintenance7Features.pNext              = chainTail;
      chainTail                               = &maintenance7Features;
      shaderObjectFeatures.pNext              = chainTail;
      chainTail                               = &shaderObjectFeatures;
      vulkan14Features.pNext                  = chainTail;
      chainTail                               = &vulkan14Features;
      vulkan13Features.pNext                  = chainTail;
      chainTail                               = &vulkan13Features;
      vulkan12Features.pNext                  = chainTail;
      chainTail                               = &vulkan12Features;
      vulkan11Features.pNext                  = chainTail;
      chainTail                               = &vulkan11Features;
      features2.pNext                         = chainTail;
      chainTail                               = &features2;

      return chainTail;
    }
  };

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
}  // namespace cfg
