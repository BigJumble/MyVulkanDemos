#pragma once
#include "features.hpp"
#include "../core/helper.hpp"

#include <vk_mem_alloc.h>
// #include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
// clang-format on

namespace init
{

  inline constexpr std::string_view AppName    = "MyApp";
  inline constexpr std::string_view EngineName = "MyEngine";

  inline vk::ApplicationInfo applicationInfo = vk::ApplicationInfo()
                                                 .setPApplicationName( AppName.data() )
                                                 .setApplicationVersion( VK_MAKE_VERSION( 1, 0, 0 ) )
                                                 .setPEngineName( EngineName.data() )
                                                 .setEngineVersion( VK_MAKE_VERSION( 1, 0, 0 ) )
                                                 .setApiVersion( VK_API_VERSION_1_4 );

  inline vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo( {}, &applicationInfo, {}, cfg::InstanceExtensions );

  namespace raii
  {

    struct Allocator
    {
      VmaAllocator allocator = nullptr;

      Allocator( const vk::raii::Instance & instance, const vk::raii::PhysicalDevice & physicalDevice, const vk::raii::Device & device )
      {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_4;
        allocatorInfo.physicalDevice         = *physicalDevice;
        allocatorInfo.device                 = *device;
        allocatorInfo.instance               = *instance;

        vmaCreateAllocator( &allocatorInfo, &allocator );
      }

      operator VmaAllocator() const
      {
          return allocator;
      }

      ~Allocator()
      {
        clear();
      }

      void clear()
      {
        if ( allocator )
        {
          vmaDestroyAllocator( allocator );
          allocator = nullptr;
        }
      }
    };

    struct DepthResources
    {
      VmaAllocator  allocator = nullptr;
      vk::Image     image;
      VmaAllocation allocation;
      vk::Device    device;
      vk::ImageView imageView;
      vk::Format    depthFormat = vk::Format::eD32Sfloat;

      DepthResources( vk::raii::Device & device, VmaAllocator allocator, vk::Extent2D extent )
      {
        this->allocator = allocator;
        this->device    = *device;

        vk::ImageCreateInfo imageInfo;
        imageInfo.setImageType( vk::ImageType::e2D )
          .setExtent( vk::Extent3D{ extent.width, extent.height, 1 } )
          .setMipLevels( 1 )
          .setArrayLayers( 1 )
          .setFormat( depthFormat )
          .setTiling( vk::ImageTiling::eOptimal )
          .setUsage( vk::ImageUsageFlagBits::eDepthStencilAttachment )
          .setSamples( vk::SampleCountFlagBits::e1 )
          .setSharingMode( vk::SharingMode::eExclusive );

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags                   = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        vmaCreateImage(allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageInfo), &allocInfo, reinterpret_cast<VkImage*>(&image), &allocation, nullptr);

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.setImage( image )
          .setViewType( vk::ImageViewType::e2D )
          .setFormat( depthFormat )
          .setSubresourceRange(
            vk::ImageSubresourceRange{}
              .setAspectMask( vk::ImageAspectFlagBits::eDepth )
              .setBaseMipLevel( 0 )
              .setLevelCount( 1 )
              .setBaseArrayLayer( 0 )
              .setLayerCount( 1 ) );

        imageView = this->device.createImageView( viewInfo );
      }

      // Add move assignment operator
      DepthResources & operator=( DepthResources && other ) noexcept
      {
        if ( this != &other )
        {
          // Clean up current resources
          clear();

          // Move from other
          allocator  = other.allocator;
          device     = other.device;
          image      = other.image;
          allocation = other.allocation;
          imageView  = other.imageView;

          // Reset other
          other.allocator  = nullptr;
          other.device     = VK_NULL_HANDLE;
          other.image      = VK_NULL_HANDLE;
          other.allocation = nullptr;
          other.imageView  = VK_NULL_HANDLE;
        }
        return *this;
      }

      // Delete copy operations
      DepthResources( const DepthResources & )             = delete;
      DepthResources & operator=( const DepthResources & ) = delete;

      ~DepthResources()
      {
        clear();
      }

      void clear()
      {
        if ( allocator )
        {
          if ( imageView )
          {
            device.destroyImageView( imageView );
            imageView = VK_NULL_HANDLE;
          }
          vmaDestroyImage( allocator, image, allocation );
        }
      }
    };



    struct ColorTarget
    {
      VmaAllocator  allocator = nullptr;
      vk::Image     image;
      VmaAllocation allocation;
      vk::Device    device;
      vk::ImageView imageView;
      vk::Format    colorFormat = vk::Format::eUndefined;
      vk::Extent2D  extent{};

      ColorTarget() = default;

      ColorTarget( vk::raii::Device & device, VmaAllocator allocator, vk::Extent2D extent, vk::Format format )
      {
        this->allocator   = allocator;
        this->device      = *device;
        this->extent      = extent;
        this->colorFormat = format;

        vk::ImageCreateInfo imageInfo;
        imageInfo.setImageType( vk::ImageType::e2D )
          .setExtent( vk::Extent3D{ extent.width, extent.height, 1 } )
          .setMipLevels( 1 )
          .setArrayLayers( 1 )
          .setFormat( colorFormat )
          .setTiling( vk::ImageTiling::eOptimal )
          .setUsage( vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc )
          .setSamples( vk::SampleCountFlagBits::e1 )
          .setSharingMode( vk::SharingMode::eExclusive );

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags                   = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        vmaCreateImage( allocator, reinterpret_cast<const VkImageCreateInfo *>( &imageInfo ), &allocInfo, reinterpret_cast<VkImage *>( &image ), &allocation, nullptr );

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.setImage( image )
          .setViewType( vk::ImageViewType::e2D )
          .setFormat( colorFormat )
          .setSubresourceRange( vk::ImageSubresourceRange{}
                                  .setAspectMask( vk::ImageAspectFlagBits::eColor )
                                  .setBaseMipLevel( 0 )
                                  .setLevelCount( 1 )
                                  .setBaseArrayLayer( 0 )
                                  .setLayerCount( 1 ) );

        imageView = this->device.createImageView( viewInfo );
      }

      ColorTarget & operator=( ColorTarget && other ) noexcept
      {
        if ( this != &other )
        {
          clear();
          allocator   = other.allocator;
          device      = other.device;
          image       = other.image;
          allocation  = other.allocation;
          imageView   = other.imageView;
          colorFormat = other.colorFormat;
          extent      = other.extent;
          other.allocator  = nullptr;
          other.device     = VK_NULL_HANDLE;
          other.image      = VK_NULL_HANDLE;
          other.allocation = nullptr;
          other.imageView  = VK_NULL_HANDLE;
          other.colorFormat = vk::Format::eUndefined;
          other.extent      = vk::Extent2D{};
        }
        return *this;
      }

      ColorTarget( const ColorTarget & )            = delete;
      ColorTarget & operator=( const ColorTarget & ) = delete;

      ~ColorTarget()
      {
        clear();
      }

      void clear()
      {
        if ( allocator )
        {
          if ( imageView )
          {
            device.destroyImageView( imageView );
            imageView = VK_NULL_HANDLE;
          }
          vmaDestroyImage( allocator, image, allocation );
        }
      }
    };

    struct ShaderBundle
    {
      vk::raii::PipelineLayout pipelineLayout;
      std::vector<vk::raii::ShaderEXT> vertexShaders;
      std::vector<vk::raii::ShaderEXT> fragmentShaders;
      
      // Current selected shader indices
      int selectedVertexShader = 0;
      int selectedFragmentShader = 0;
      
      // Shader names for ImGui display
      std::vector<std::string> vertexShaderNames;
      std::vector<std::string> fragmentShaderNames;

      ShaderBundle(const vk::raii::Device& device, 
                   const std::vector<std::string>& vertShaderNames,
                   const std::vector<std::string>& fragShaderNames,
                   const vk::PushConstantRange& pushConstantRange = {})
        : pipelineLayout(createPipelineLayout(device, pushConstantRange))
        , vertexShaderNames(vertShaderNames)
        , fragmentShaderNames(fragShaderNames)
      {
        // Create vertex shaders
        for (const auto& shaderName : vertShaderNames) {
          vertexShaders.emplace_back(createShader(device, shaderName, vk::ShaderStageFlagBits::eVertex, pushConstantRange));
        }
        
        // Create fragment shaders
        for (const auto& shaderName : fragShaderNames) {
          fragmentShaders.emplace_back(createShader(device, shaderName, vk::ShaderStageFlagBits::eFragment, pushConstantRange));
        }
      }

      // Get currently selected vertex shader
      vk::raii::ShaderEXT& getCurrentVertexShader() {
        return vertexShaders[selectedVertexShader];
      }

      // Get currently selected fragment shader
      vk::raii::ShaderEXT& getCurrentFragmentShader() {
        return fragmentShaders[selectedFragmentShader];
      }

      // Set selected vertex shader by index
      void setVertexShader(int index) {
        if (index >= 0 && index < static_cast<int>(vertexShaders.size())) {
          selectedVertexShader = index;
        }
      }

      // Set selected fragment shader by index
      void setFragmentShader(int index) {
        if (index >= 0 && index < static_cast<int>(fragmentShaders.size())) {
          selectedFragmentShader = index;
        }
      }

      // Get shader count for ImGui
      int getVertexShaderCount() const { return static_cast<int>(vertexShaders.size()); }
      int getFragmentShaderCount() const { return static_cast<int>(fragmentShaders.size()); }

    private:
      vk::raii::PipelineLayout createPipelineLayout(const vk::raii::Device& device, const vk::PushConstantRange& pushConstantRange) {
        vk::PipelineLayoutCreateInfo layoutInfo{};
        if (pushConstantRange.size > 0) {
          layoutInfo.setPushConstantRangeCount(1).setPPushConstantRanges(&pushConstantRange);
        }
        return vk::raii::PipelineLayout(device, layoutInfo);
      }

      vk::raii::ShaderEXT createShader(const vk::raii::Device& device, 
                                       const std::string& shaderName,
                                       vk::ShaderStageFlagBits stage,
                                       const vk::PushConstantRange& pushConstantRange) {
        std::vector<uint32_t> shaderCode = core::help::getShaderCode(shaderName);
        
        vk::ShaderCreateInfoEXT shaderInfo{};
        shaderInfo.setStage(stage)
                  .setCodeType(vk::ShaderCodeTypeEXT::eSpirv)
                  .setPCode(shaderCode.data())
                  .setPName("main")
                  .setCodeSize(shaderCode.size() * sizeof(uint32_t));
        
        if (pushConstantRange.size > 0) {
          shaderInfo.setPushConstantRangeCount(1).setPPushConstantRanges(&pushConstantRange);
        }
        
        // Set next stage for vertex shaders
        if (stage == vk::ShaderStageFlagBits::eVertex) {
          shaderInfo.setNextStage(vk::ShaderStageFlagBits::eFragment);
        }
        
        return vk::raii::ShaderEXT(device, shaderInfo);
      }
    };

    struct IMGUI
    {
      vk::raii::DescriptorPool descriptorPool;
      
      IMGUI(vk::raii::Device const& device,
            vk::raii::Instance const& instance,
            vk::raii::PhysicalDevice const& physicalDevice,
            uint32_t queueFamily,
            vk::raii::Queue const& queue,
            GLFWwindow* window,
            uint32_t minImageCount,
            uint32_t imageCount,
            vk::Format swapchainFormat,
            vk::Format depthFormat)
        : descriptorPool(createDescriptorPool(device))
      {
        // Initialize ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        
        // Initialize GLFW implementation
        ImGui_ImplGlfw_InitForVulkan(window, true);
        
        // Initialize Vulkan implementation
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = *instance;
        initInfo.PhysicalDevice = *physicalDevice;
        initInfo.Device = *device;
        initInfo.QueueFamily = queueFamily;
        initInfo.Queue = *queue;
        initInfo.DescriptorPool = *descriptorPool;
        initInfo.RenderPass = VK_NULL_HANDLE;
        initInfo.MinImageCount = minImageCount;
        initInfo.ImageCount = imageCount;
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.UseDynamicRendering = true;
        
        // Set up dynamic rendering info
        vk::PipelineRenderingCreateInfoKHR pipelineRenderingInfo{};
        pipelineRenderingInfo.colorAttachmentCount = 1;
        pipelineRenderingInfo.pColorAttachmentFormats = &swapchainFormat;
        pipelineRenderingInfo.depthAttachmentFormat = depthFormat;
        
        initInfo.PipelineRenderingCreateInfo = pipelineRenderingInfo;
        
        ImGui_ImplVulkan_Init(&initInfo);
      }
      
      ~IMGUI()
      {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        // descriptorPool is automatically destroyed by vk::raii::DescriptorPool
      }
      
      // Delete copy operations
      IMGUI(IMGUI const&) = delete;
      IMGUI& operator=(IMGUI const&) = delete;
      
      // Allow move operations
      IMGUI(IMGUI&& other) noexcept = default;
      IMGUI& operator=(IMGUI&& other) noexcept = default;
      
    private:
      static vk::raii::DescriptorPool createDescriptorPool(vk::raii::Device const& device)
      {
        std::array<vk::DescriptorPoolSize, 11> poolSizes = {
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
          vk::DescriptorPoolSize{ vk::DescriptorType::eInputAttachment, 1000 }
        };
        
        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets = 1000 * static_cast<uint32_t>(poolSizes.size());
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        
        return vk::raii::DescriptorPool{ device, poolInfo };
      }
    };
  }  // namespace raii
}  // namespace init
