#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ui {

// Resource types
enum class ResourceType {
  Shader,
  Texture,
  Buffer,
  Sampler,
  Pipeline
};

// Resource entry
struct Resource {
  std::string  name;
  ResourceType type;
};

// Shader creation state
struct ShaderCreationState {
  char shaderFileName[256] = "";
  int  shaderTypeIndex     = 0;
};

// Texture creation state
struct TextureCreationState {
  char textureName[256] = "";
  int  formatIndex      = 0; // 0=RGBA8, 1=RGBA16F, 2=RGBA32F, 3=R8, 4=RG8
  int  width            = 1024;
  int  height           = 1024;
};

// Buffer creation state
struct BufferCreationState {
  char     bufferName[256] = "";
  int      bufferTypeIndex = 0; // 0=Storage Buffer, 1=Uniform Buffer, 2=Vertex Buffer, 3=Index Buffer
  uint32_t size            = 1024;
};

// Sampler creation state
struct SamplerCreationState {
  char samplerName[256]   = "";
  int  filterIndex        = 0; // 0=Linear, 1=Nearest
  int  addressModeIndex   = 0; // 0=Repeat, 1=Clamp to Edge, 2=Clamp to Border, 3=Mirror Repeat
  int  mipmapModeIndex    = 0; // 0=Linear, 1=Nearest
};

// Pipeline creation state
struct PipelineCreationState {
  char pipelineName[256] = "";
  int  pipelineTypeIndex = 0; // 0=Graphics Pipeline, 1=Compute Pipeline, 2=Ray Tracing Pipeline
};

// Resource manager UI state
struct ResourceManagerState {
  std::vector<Resource>    resources;
  ShaderCreationState      shaderCreation;
  TextureCreationState     textureCreation;
  BufferCreationState      bufferCreation;
  SamplerCreationState     samplerCreation;
  PipelineCreationState    pipelineCreation;
  bool                     showCreatePopup      = false;
  bool                     openCreationModal    = false;
  ResourceType             selectedResourceType = ResourceType::Shader;
  
  // Project management
  std::string              currentProjectName;
  std::string              currentProjectPath;
};

// Descriptor binding information from reflection
struct DescriptorBinding {
  uint32_t    set;
  uint32_t    binding;
  std::string name;
  std::string type; // uniform, storage, sampler, etc.
  std::string assignedResource; // Name of the resource assigned to this binding
};

// Shader stage assignment
struct ShaderStageAssignment {
  std::string shaderName;  // Name of the compiled shader
  std::string shaderPath;  // Path to compiled SPIR-V
  bool        isAssigned = false;
};

// Pipeline configuration
struct PipelineConfig {
  std::string               pipelineName;
  ShaderStageAssignment     vertexShader;
  ShaderStageAssignment     fragmentShader;
  ShaderStageAssignment     computeShader;
  std::vector<DescriptorBinding> descriptorBindings;
  bool                      isExpanded = false;
};

// Main loop state
struct MainLoopState {
  std::vector<PipelineConfig> pipelines;
  int                         selectedPipelineIndex = -1;
  bool                        showCompileLog        = false;
  std::string                 compileLog;
};

void renderStatsWindow();

void renderResourceManagerWindow( ResourceManagerState & state );

void renderMainLoopWindow( MainLoopState & mainLoopState, ResourceManagerState & resourceState );

// Project management functions
bool saveProject( const ResourceManagerState & state, const std::string & projectPath );
bool loadProject( ResourceManagerState & state, const std::string & projectPath );

// Shader compilation
bool compileAllShaders( std::string & logOutput );

// Reflection
bool reflectShader( const std::string & spirvPath, std::vector<DescriptorBinding> & bindings );

} // namespace ui
