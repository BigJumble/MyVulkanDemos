#include "ui.hpp"
#include "imgui.h"
#include "shaderc/env.h"
#include <fstream>
#include <filesystem>
#include <print>
#include <nlohmann/json.hpp>
#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>

using json = nlohmann::json;

namespace ui {

// Helper function to get resource type name
static const char * getResourceTypeName( ResourceType type )
{
  switch ( type )
  {
    case ResourceType::Shader:   return "Shader";
    case ResourceType::Texture:  return "Texture";
    case ResourceType::Buffer:   return "Buffer";
    case ResourceType::Sampler:  return "Sampler";
    case ResourceType::Pipeline: return "Pipeline";
    default:                     return "Unknown";
  }
}

void renderStatsWindow()
{
  ImGui::Begin( "Stats" );
  ImGui::Text( "FPS: %.1f", ImGui::GetIO().Framerate );
  ImGui::Text( "Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate );
  ImGui::End();
}

// Render shader creation dialog
static void renderShaderCreationDialog( ShaderCreationState & shaderState, std::vector<Resource> & resources )
{
  static const char * shaderExtensions[] = {
    ".vert", ".frag", ".comp",
    ".rgen", ".rchit", ".rmiss", ".rahit", ".rint", ".rcall"
  };
  static const char * shaderTypeNames[] = {
    "Vertex", "Fragment", "Compute",
    "Ray Gen", "Ray Closest Hit", "Ray Miss", "Ray Any Hit", "Ray Intersection", "Ray Callable"
  };

  ImGui::Text( "Create Shader" );
  ImGui::Separator();

  ImGui::InputText( "Filename", shaderState.shaderFileName, sizeof( shaderState.shaderFileName ) );

  ImGui::Text( "Shader Type:" );
  for ( int i = 0; i < 9; ++i )
  {
    if ( ImGui::RadioButton( shaderTypeNames[i], &shaderState.shaderTypeIndex, i ) )
    {
      // Radio button clicked
    }
    if ( i == 2 || i == 5 )
    {
      ImGui::Separator();
    }
  }

  ImGui::Separator();

  if ( ImGui::Button( "Create", ImVec2( 100, 0 ) ) )
  {
    if ( strlen( shaderState.shaderFileName ) > 0 )
    {
      // Ensure shaders directory exists
      std::filesystem::create_directories( "./shaders" );

      // Construct full filename with extension
      std::string fullFileName = "./shaders/";
      fullFileName += shaderState.shaderFileName;
      fullFileName += shaderExtensions[shaderState.shaderTypeIndex];

      // Create the file with a basic template
      std::ofstream shaderFile( fullFileName );
      if ( shaderFile.is_open() )
      {
        shaderFile << "#version 450\n\n";

        // Add appropriate template based on shader type
        if ( shaderState.shaderTypeIndex == 0 )
        { // Vertex
          shaderFile << "layout(location = 0) out vec3 fragColor;\n\n";
          shaderFile << "void main() {\n";
          shaderFile << "    // Vertex shader code\n";
          shaderFile << "}\n";
        }
        else if ( shaderState.shaderTypeIndex == 1 )
        { // Fragment
          shaderFile << "layout(location = 0) in vec3 fragColor;\n";
          shaderFile << "layout(location = 0) out vec4 outColor;\n\n";
          shaderFile << "void main() {\n";
          shaderFile << "    // Fragment shader code\n";
          shaderFile << "    outColor = vec4(fragColor, 1.0);\n";
          shaderFile << "}\n";
        }
        else if ( shaderState.shaderTypeIndex == 2 )
        { // Compute
          shaderFile << "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n\n";
          shaderFile << "void main() {\n";
          shaderFile << "    // Compute shader code\n";
          shaderFile << "}\n";
        }
        else if ( shaderState.shaderTypeIndex == 3 )
        { // Ray Gen
          shaderFile << "void main() {\n";
          shaderFile << "    // Ray generation shader code\n";
          shaderFile << "}\n";
        }
        else if ( shaderState.shaderTypeIndex == 4 )
        { // Ray Closest Hit
          shaderFile << "void main() {\n";
          shaderFile << "    // Ray closest hit shader code\n";
          shaderFile << "}\n";
        }
        else if ( shaderState.shaderTypeIndex == 5 )
        { // Ray Miss
          shaderFile << "void main() {\n";
          shaderFile << "    // Ray miss shader code\n";
          shaderFile << "}\n";
        }
        else if ( shaderState.shaderTypeIndex == 6 )
        { // Ray Any Hit
          shaderFile << "void main() {\n";
          shaderFile << "    // Ray any hit shader code\n";
          shaderFile << "}\n";
        }
        else if ( shaderState.shaderTypeIndex == 7 )
        { // Ray Intersection
          shaderFile << "void main() {\n";
          shaderFile << "    // Ray intersection shader code\n";
          shaderFile << "}\n";
        }
        else if ( shaderState.shaderTypeIndex == 8 )
        { // Ray Callable
          shaderFile << "void main() {\n";
          shaderFile << "    // Ray callable shader code\n";
          shaderFile << "}\n";
        }

        shaderFile.close();
        std::println( "Created shader file: {}", fullFileName );

        // Add to resource list
        resources.push_back( { shaderState.shaderFileName, ResourceType::Shader } );

        // Clear filename after creation
        shaderState.shaderFileName[0] = '\0';
      }
      else
      {
        std::println( "Failed to create shader file: {}", fullFileName );
      }

      ImGui::CloseCurrentPopup();
    }
  }

  ImGui::SameLine();
  if ( ImGui::Button( "Cancel", ImVec2( 100, 0 ) ) )
  {
    ImGui::CloseCurrentPopup();
  }
}

// Render texture creation dialog
static void renderTextureCreationDialog( TextureCreationState & textureState, std::vector<Resource> & resources )
{
  static const char * formatNames[] = {
    "RGBA8", "RGBA16F", "RGBA32F", "R8", "RG8"
  };

  ImGui::Text( "Create Texture" );
  ImGui::Separator();

  ImGui::InputText( "Texture Name", textureState.textureName, sizeof( textureState.textureName ) );

  // Format selection
  ImGui::Text( "Format:" );
  ImGui::Combo( "##Format", &textureState.formatIndex, formatNames, IM_ARRAYSIZE( formatNames ) );

  // Size inputs
  ImGui::Text( "Size:" );
  ImGui::InputInt( "Width", &textureState.width );
  ImGui::InputInt( "Height", &textureState.height );

  // Clamp to reasonable values
  if ( textureState.width < 1 ) textureState.width = 1;
  if ( textureState.height < 1 ) textureState.height = 1;
  if ( textureState.width > 8192 ) textureState.width = 8192;
  if ( textureState.height > 8192 ) textureState.height = 8192;

  ImGui::Separator();

  if ( ImGui::Button( "Create", ImVec2( 100, 0 ) ) )
  {
    if ( strlen( textureState.textureName ) > 0 )
    {
      std::println( "Created texture: {} ({}x{}, {})", 
                    textureState.textureName, 
                    textureState.width, 
                    textureState.height, 
                    formatNames[textureState.formatIndex] );
      resources.push_back( { textureState.textureName, ResourceType::Texture } );
      textureState.textureName[0] = '\0';
      textureState.width = 1024;
      textureState.height = 1024;
      textureState.formatIndex = 0;
      ImGui::CloseCurrentPopup();
    }
  }

  ImGui::SameLine();
  if ( ImGui::Button( "Cancel", ImVec2( 100, 0 ) ) )
  {
    ImGui::CloseCurrentPopup();
  }
}

// Render buffer creation dialog
static void renderBufferCreationDialog( BufferCreationState & bufferState, std::vector<Resource> & resources )
{
  static const char * bufferTypeNames[] = {
    "Storage Buffer", "Uniform Buffer", "Vertex Buffer", "Index Buffer"
  };

  ImGui::Text( "Create Buffer" );
  ImGui::Separator();

  ImGui::InputText( "Buffer Name", bufferState.bufferName, sizeof( bufferState.bufferName ) );

  // Buffer type selection
  ImGui::Text( "Type:" );
  ImGui::Combo( "##BufferType", &bufferState.bufferTypeIndex, bufferTypeNames, IM_ARRAYSIZE( bufferTypeNames ) );

  // Size input
  ImGui::Text( "Size (bytes):" );
  int sizeInt = static_cast<int>( bufferState.size );
  ImGui::InputInt( "##BufferSize", &sizeInt );
  
  // Clamp to reasonable values
  if ( sizeInt < 1 ) sizeInt = 1;
  if ( sizeInt > 1073741824 ) sizeInt = 1073741824; // 1GB max
  bufferState.size = static_cast<uint32_t>( sizeInt );

  ImGui::Separator();

  if ( ImGui::Button( "Create", ImVec2( 100, 0 ) ) )
  {
    if ( strlen( bufferState.bufferName ) > 0 )
    {
      std::println( "Created buffer: {} ({} bytes, {})", 
                    bufferState.bufferName, 
                    bufferState.size, 
                    bufferTypeNames[bufferState.bufferTypeIndex] );
      resources.push_back( { bufferState.bufferName, ResourceType::Buffer } );
      bufferState.bufferName[0] = '\0';
      bufferState.size = 1024;
      bufferState.bufferTypeIndex = 0;
      ImGui::CloseCurrentPopup();
    }
  }

  ImGui::SameLine();
  if ( ImGui::Button( "Cancel", ImVec2( 100, 0 ) ) )
  {
    ImGui::CloseCurrentPopup();
  }
}

// Render sampler creation dialog
static void renderSamplerCreationDialog( SamplerCreationState & samplerState, std::vector<Resource> & resources )
{
  static const char * filterNames[] = {
    "Linear", "Nearest"
  };
  static const char * addressModeNames[] = {
    "Repeat", "Clamp to Edge", "Clamp to Border", "Mirror Repeat"
  };
  static const char * mipmapModeNames[] = {
    "Linear", "Nearest"
  };

  ImGui::Text( "Create Sampler" );
  ImGui::Separator();

  ImGui::InputText( "Sampler Name", samplerState.samplerName, sizeof( samplerState.samplerName ) );

  // Filter selection
  ImGui::Text( "Filter:" );
  ImGui::Combo( "##Filter", &samplerState.filterIndex, filterNames, IM_ARRAYSIZE( filterNames ) );

  // Address mode selection
  ImGui::Text( "Address Mode:" );
  ImGui::Combo( "##AddressMode", &samplerState.addressModeIndex, addressModeNames, IM_ARRAYSIZE( addressModeNames ) );

  // Mipmap mode selection
  ImGui::Text( "Mipmap Mode:" );
  ImGui::Combo( "##MipmapMode", &samplerState.mipmapModeIndex, mipmapModeNames, IM_ARRAYSIZE( mipmapModeNames ) );

  ImGui::Separator();

  if ( ImGui::Button( "Create", ImVec2( 100, 0 ) ) )
  {
    if ( strlen( samplerState.samplerName ) > 0 )
    {
      std::println( "Created sampler: {} (Filter: {}, Address: {}, Mipmap: {})", 
                    samplerState.samplerName, 
                    filterNames[samplerState.filterIndex],
                    addressModeNames[samplerState.addressModeIndex],
                    mipmapModeNames[samplerState.mipmapModeIndex] );
      resources.push_back( { samplerState.samplerName, ResourceType::Sampler } );
      samplerState.samplerName[0] = '\0';
      samplerState.filterIndex = 0;
      samplerState.addressModeIndex = 0;
      samplerState.mipmapModeIndex = 0;
      ImGui::CloseCurrentPopup();
    }
  }

  ImGui::SameLine();
  if ( ImGui::Button( "Cancel", ImVec2( 100, 0 ) ) )
  {
    ImGui::CloseCurrentPopup();
  }
}

// Render pipeline creation dialog
static void renderPipelineCreationDialog( PipelineCreationState & pipelineState, std::vector<Resource> & resources )
{
  static const char * pipelineTypeNames[] = {
    "Graphics Pipeline", "Compute Pipeline", "Ray Tracing Pipeline"
  };

  ImGui::Text( "Create Pipeline" );
  ImGui::Separator();

  ImGui::InputText( "Pipeline Name", pipelineState.pipelineName, sizeof( pipelineState.pipelineName ) );

  // Pipeline type selection
  ImGui::Text( "Type:" );
  ImGui::Combo( "##PipelineType", &pipelineState.pipelineTypeIndex, pipelineTypeNames, IM_ARRAYSIZE( pipelineTypeNames ) );

  ImGui::Separator();

  if ( ImGui::Button( "Create", ImVec2( 100, 0 ) ) )
  {
    if ( strlen( pipelineState.pipelineName ) > 0 )
    {
      std::println( "Created pipeline: {} ({})", 
                    pipelineState.pipelineName, 
                    pipelineTypeNames[pipelineState.pipelineTypeIndex] );
      resources.push_back( { pipelineState.pipelineName, ResourceType::Pipeline } );
      pipelineState.pipelineName[0] = '\0';
      pipelineState.pipelineTypeIndex = 0;
      ImGui::CloseCurrentPopup();
    }
  }

  ImGui::SameLine();
  if ( ImGui::Button( "Cancel", ImVec2( 100, 0 ) ) )
  {
    ImGui::CloseCurrentPopup();
  }
}

void renderResourceManagerWindow( ResourceManagerState & state )
{
  ImGui::Begin( "Resource Manager" );

  // Project management section
  ImGui::Text( "Project: %s", state.currentProjectName.empty() ? "Unsaved" : state.currentProjectName.c_str() );
  ImGui::Separator();

  static char projectPathBuffer[512] = "./projects/MyProject";

  ImGui::PushItemWidth( 300 );
  ImGui::InputText( "##ProjectPath", projectPathBuffer, sizeof( projectPathBuffer ) );
  ImGui::PopItemWidth();
  
  ImGui::SameLine();
  if ( ImGui::Button( "Save Project", ImVec2( 100, 0 ) ) )
  {
    if ( saveProject( state, projectPathBuffer ) )
    {
      state.currentProjectPath = projectPathBuffer;
      state.currentProjectName = std::filesystem::path( projectPathBuffer ).filename().string();
    }
  }

  ImGui::SameLine();
  if ( ImGui::Button( "Load Project", ImVec2( 100, 0 ) ) )
  {
    loadProject( state, projectPathBuffer );
  }

  ImGui::Spacing();
  ImGui::Separator();

  // Resource list section
  ImGui::Text( "Resources (%zu)", state.resources.size() );
  ImGui::Separator();

  // Display resources in a table
  if ( ImGui::BeginTable( "ResourceTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg ) )
  {
    ImGui::TableSetupColumn( "Name" );
    ImGui::TableSetupColumn( "Type" );
    ImGui::TableSetupColumn( "Actions" );
    ImGui::TableHeadersRow();

    for ( size_t i = 0; i < state.resources.size(); ++i )
    {
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex( 0 );
      ImGui::Text( "%s", state.resources[i].name.c_str() );

      ImGui::TableSetColumnIndex( 1 );
      ImGui::Text( "%s", getResourceTypeName( state.resources[i].type ) );

      ImGui::TableSetColumnIndex( 2 );
      ImGui::PushID( static_cast<int>( i ) );
      if ( ImGui::SmallButton( "Delete" ) )
      {
        state.resources.erase( state.resources.begin() + i );
        ImGui::PopID();
        break;
      }
      ImGui::PopID();
    }

    ImGui::EndTable();
  }

  ImGui::Spacing();

  // Add resource button
  if ( ImGui::Button( "Add Resource", ImVec2( 150, 30 ) ) )
  {
    state.showCreatePopup = true;
    ImGui::OpenPopup( "SelectResourceType" );
  }

  // Resource type selection popup
  if ( ImGui::BeginPopup( "SelectResourceType" ) )
  {
    ImGui::Text( "Select Resource Type" );
    ImGui::Separator();

    if ( ImGui::Selectable( "Shader" ) )
    {
      state.selectedResourceType = ResourceType::Shader;
      state.openCreationModal    = true;
      ImGui::CloseCurrentPopup();
    }
    if ( ImGui::Selectable( "Texture" ) )
    {
      state.selectedResourceType = ResourceType::Texture;
      state.openCreationModal    = true;
      ImGui::CloseCurrentPopup();
    }
    if ( ImGui::Selectable( "Buffer" ) )
    {
      state.selectedResourceType = ResourceType::Buffer;
      state.openCreationModal    = true;
      ImGui::CloseCurrentPopup();
    }
    if ( ImGui::Selectable( "Sampler" ) )
    {
      state.selectedResourceType = ResourceType::Sampler;
      state.openCreationModal    = true;
      ImGui::CloseCurrentPopup();
    }
    if ( ImGui::Selectable( "Pipeline" ) )
    {
      state.selectedResourceType = ResourceType::Pipeline;
      state.openCreationModal    = true;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }

  ImGui::End();

  // Open the appropriate modal based on state
  if ( state.openCreationModal )
  {
    state.openCreationModal = false;
    switch ( state.selectedResourceType )
    {
      case ResourceType::Shader:
        ImGui::OpenPopup( "CreateShader" );
        break;
      case ResourceType::Texture:
        ImGui::OpenPopup( "CreateTexture" );
        break;
      case ResourceType::Buffer:
        ImGui::OpenPopup( "CreateBuffer" );
        break;
      case ResourceType::Sampler:
        ImGui::OpenPopup( "CreateSampler" );
        break;
      case ResourceType::Pipeline:
        ImGui::OpenPopup( "CreatePipeline" );
        break;
    }
  }

  // Resource creation modal windows (outside of main window)
  if ( ImGui::BeginPopupModal( "CreateShader", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
  {
    renderShaderCreationDialog( state.shaderCreation, state.resources );
    ImGui::EndPopup();
  }

  if ( ImGui::BeginPopupModal( "CreateTexture", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
  {
    renderTextureCreationDialog( state.textureCreation, state.resources );
    ImGui::EndPopup();
  }

  if ( ImGui::BeginPopupModal( "CreateBuffer", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
  {
    renderBufferCreationDialog( state.bufferCreation, state.resources );
    ImGui::EndPopup();
  }

  if ( ImGui::BeginPopupModal( "CreateSampler", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
  {
    renderSamplerCreationDialog( state.samplerCreation, state.resources );
    ImGui::EndPopup();
  }

  if ( ImGui::BeginPopupModal( "CreatePipeline", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
  {
    renderPipelineCreationDialog( state.pipelineCreation, state.resources );
    ImGui::EndPopup();
  }
}

// Convert ResourceType to string
static std::string resourceTypeToString( ResourceType type )
{
  switch ( type )
  {
    case ResourceType::Shader:   return "Shader";
    case ResourceType::Texture:  return "Texture";
    case ResourceType::Buffer:   return "Buffer";
    case ResourceType::Sampler:  return "Sampler";
    case ResourceType::Pipeline: return "Pipeline";
    default:                     return "Unknown";
  }
}

// Convert string to ResourceType
static ResourceType stringToResourceType( const std::string & typeStr )
{
  if ( typeStr == "Shader" ) return ResourceType::Shader;
  if ( typeStr == "Texture" ) return ResourceType::Texture;
  if ( typeStr == "Buffer" ) return ResourceType::Buffer;
  if ( typeStr == "Sampler" ) return ResourceType::Sampler;
  if ( typeStr == "Pipeline" ) return ResourceType::Pipeline;
  return ResourceType::Shader;
}

bool saveProject( const ResourceManagerState & state, const std::string & projectPath )
{
  try
  {
    // Create project directory
    std::filesystem::path projPath( projectPath );
    std::filesystem::create_directories( projPath );
    
    // Create shaders subdirectory
    std::filesystem::path shadersPath = projPath / "shaders";
    std::filesystem::create_directories( shadersPath );
    
    // Build JSON object
    json projectJson;
    projectJson["version"] = "1.0";
    projectJson["name"] = projPath.filename().string();
    
    // Save resources
    json resourcesJson = json::array();
    for ( const auto & resource : state.resources )
    {
      json resourceJson;
      resourceJson["name"] = resource.name;
      resourceJson["type"] = resourceTypeToString( resource.type );
      resourcesJson.push_back( resourceJson );
      
      // If it's a shader, copy the shader file
      if ( resource.type == ResourceType::Shader )
      {
        // Try to find and copy shader files
        std::filesystem::path shadersSrc = "./shaders";
        if ( std::filesystem::exists( shadersSrc ) )
        {
          for ( const auto & entry : std::filesystem::directory_iterator( shadersSrc ) )
          {
            if ( entry.path().stem().string() == resource.name )
            {
              std::filesystem::path destPath = shadersPath / entry.path().filename();
              std::filesystem::copy_file( entry.path(), destPath, std::filesystem::copy_options::overwrite_existing );
              std::println( "Copied shader: {} -> {}", entry.path().string(), destPath.string() );
            }
          }
        }
      }
    }
    projectJson["resources"] = resourcesJson;
    
    // Save creation states (optional, for preserving dialog state)
    json creationStates;
    creationStates["shader"]["typeIndex"] = state.shaderCreation.shaderTypeIndex;
    creationStates["texture"]["formatIndex"] = state.textureCreation.formatIndex;
    creationStates["texture"]["width"] = state.textureCreation.width;
    creationStates["texture"]["height"] = state.textureCreation.height;
    creationStates["buffer"]["typeIndex"] = state.bufferCreation.bufferTypeIndex;
    creationStates["buffer"]["size"] = state.bufferCreation.size;
    creationStates["sampler"]["filterIndex"] = state.samplerCreation.filterIndex;
    creationStates["sampler"]["addressModeIndex"] = state.samplerCreation.addressModeIndex;
    creationStates["sampler"]["mipmapModeIndex"] = state.samplerCreation.mipmapModeIndex;
    creationStates["pipeline"]["typeIndex"] = state.pipelineCreation.pipelineTypeIndex;
    projectJson["creationStates"] = creationStates;
    
    // Write JSON to file
    std::filesystem::path jsonPath = projPath / "project.json";
    std::ofstream jsonFile( jsonPath );
    if ( !jsonFile.is_open() )
    {
      std::println( "Failed to create project.json file" );
      return false;
    }
    
    jsonFile << projectJson.dump( 2 );
    jsonFile.close();
    
    std::println( "Project saved successfully to: {}", projectPath );
    return true;
  }
  catch ( const std::exception & e )
  {
    std::println( "Error saving project: {}", e.what() );
    return false;
  }
}

bool loadProject( ResourceManagerState & state, const std::string & projectPath )
{
  try
  {
    std::filesystem::path projPath( projectPath );
    std::filesystem::path jsonPath = projPath / "project.json";
    
    if ( !std::filesystem::exists( jsonPath ) )
    {
      std::println( "Project file not found: {}", jsonPath.string() );
      return false;
    }
    
    // Read JSON file
    std::ifstream jsonFile( jsonPath );
    if ( !jsonFile.is_open() )
    {
      std::println( "Failed to open project.json file" );
      return false;
    }
    
    json projectJson;
    jsonFile >> projectJson;
    jsonFile.close();
    
    // Clear current state
    state.resources.clear();
    
    // Load resources
    if ( projectJson.contains( "resources" ) )
    {
      for ( const auto & resourceJson : projectJson["resources"] )
      {
        Resource resource;
        resource.name = resourceJson["name"];
        resource.type = stringToResourceType( resourceJson["type"] );
        state.resources.push_back( resource );
      }
    }
    
    // Copy shaders from project to working directory
    std::filesystem::path projectShaders = projPath / "shaders";
    if ( std::filesystem::exists( projectShaders ) )
    {
      std::filesystem::create_directories( "./shaders" );
      
      for ( const auto & entry : std::filesystem::directory_iterator( projectShaders ) )
      {
        if ( entry.is_regular_file() )
        {
          std::filesystem::path destPath = std::filesystem::path( "./shaders" ) / entry.path().filename();
          std::filesystem::copy_file( entry.path(), destPath, std::filesystem::copy_options::overwrite_existing );
          std::println( "Loaded shader: {} -> {}", entry.path().string(), destPath.string() );
        }
      }
    }
    
    // Load creation states (optional)
    if ( projectJson.contains( "creationStates" ) )
    {
      const auto & cs = projectJson["creationStates"];
      
      if ( cs.contains( "shader" ) && cs["shader"].contains( "typeIndex" ) )
        state.shaderCreation.shaderTypeIndex = cs["shader"]["typeIndex"];
      
      if ( cs.contains( "texture" ) )
      {
        if ( cs["texture"].contains( "formatIndex" ) )
          state.textureCreation.formatIndex = cs["texture"]["formatIndex"];
        if ( cs["texture"].contains( "width" ) )
          state.textureCreation.width = cs["texture"]["width"];
        if ( cs["texture"].contains( "height" ) )
          state.textureCreation.height = cs["texture"]["height"];
      }
      
      if ( cs.contains( "buffer" ) )
      {
        if ( cs["buffer"].contains( "typeIndex" ) )
          state.bufferCreation.bufferTypeIndex = cs["buffer"]["typeIndex"];
        if ( cs["buffer"].contains( "size" ) )
          state.bufferCreation.size = cs["buffer"]["size"];
      }
      
      if ( cs.contains( "sampler" ) )
      {
        if ( cs["sampler"].contains( "filterIndex" ) )
          state.samplerCreation.filterIndex = cs["sampler"]["filterIndex"];
        if ( cs["sampler"].contains( "addressModeIndex" ) )
          state.samplerCreation.addressModeIndex = cs["sampler"]["addressModeIndex"];
        if ( cs["sampler"].contains( "mipmapModeIndex" ) )
          state.samplerCreation.mipmapModeIndex = cs["sampler"]["mipmapModeIndex"];
      }
      
      if ( cs.contains( "pipeline" ) && cs["pipeline"].contains( "typeIndex" ) )
        state.pipelineCreation.pipelineTypeIndex = cs["pipeline"]["typeIndex"];
    }
    
    // Update current project info
    state.currentProjectPath = projectPath;
    state.currentProjectName = projPath.filename().string();
    
    std::println( "Project loaded successfully from: {}", projectPath );
    return true;
  }
  catch ( const std::exception & e )
  {
    std::println( "Error loading project: {}", e.what() );
    return false;
  }
}

// Helper function to determine shader kind from file extension
static shaderc_shader_kind getShaderKind( const std::string & filename )
{
  std::filesystem::path filePath( filename );
  std::string ext = filePath.extension().string();
  
  if ( ext == ".vert" ) return shaderc_vertex_shader;
  if ( ext == ".frag" ) return shaderc_fragment_shader;
  if ( ext == ".comp" ) return shaderc_compute_shader;
  if ( ext == ".geom" ) return shaderc_geometry_shader;
  if ( ext == ".tesc" ) return shaderc_tess_control_shader;
  if ( ext == ".tese" ) return shaderc_tess_evaluation_shader;
  if ( ext == ".rgen" ) return shaderc_raygen_shader;
  if ( ext == ".rchit" ) return shaderc_closesthit_shader;
  if ( ext == ".rmiss" ) return shaderc_miss_shader;
  if ( ext == ".rahit" ) return shaderc_anyhit_shader;
  if ( ext == ".rint" ) return shaderc_intersection_shader;
  if ( ext == ".rcall" ) return shaderc_callable_shader;
  
  return shaderc_glsl_infer_from_source;
}

bool compileAllShaders( std::string & logOutput )
{
  logOutput.clear();
  bool allSuccess = true;
  
  // Create compiled directory
  std::filesystem::create_directories( "./compiled" );
  
  // Find all shader files in ./shaders directory
  std::filesystem::path shadersDir = "./shaders";
  if ( !std::filesystem::exists( shadersDir ) )
  {
    logOutput += "Error: ./shaders directory does not exist\n";
    return false;
  }
  
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;
  
  // Set optimization level
  options.SetOptimizationLevel( shaderc_optimization_level_performance );
  options.SetTargetEnvironment( shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4 );
  options.SetTargetSpirv( shaderc_spirv_version_1_6 );
  
  int compiledCount = 0;
  int errorCount = 0;
  
  for ( const auto & entry : std::filesystem::directory_iterator( shadersDir ) )
  {
    if ( !entry.is_regular_file() )
      continue;
    
    std::string filename = entry.path().filename().string();
    std::string fullPath = entry.path().string();
    
    // Read shader source
    std::ifstream shaderFile( fullPath );
    if ( !shaderFile.is_open() )
    {
      logOutput += "Error: Could not open " + filename + "\n";
      errorCount++;
      allSuccess = false;
      continue;
    }
    
    std::string shaderSource( ( std::istreambuf_iterator<char>( shaderFile ) ),
                               std::istreambuf_iterator<char>() );
    shaderFile.close();
    
    // Compile shader
    shaderc_shader_kind kind = getShaderKind( filename );
    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv( 
      shaderSource, kind, filename.c_str(), options );
    
    if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
    {
      logOutput += "Error compiling " + filename + ":\n";
      logOutput += result.GetErrorMessage() + "\n\n";
      errorCount++;
      allSuccess = false;
      continue;
    }
    
    // Write compiled SPIR-V to file
    std::string outputPath = "./compiled/" + filename + ".spv";
    std::ofstream outputFile( outputPath, std::ios::binary );
    if ( !outputFile.is_open() )
    {
      logOutput += "Error: Could not write to " + outputPath + "\n";
      errorCount++;
      allSuccess = false;
      continue;
    }
    
    std::vector<uint32_t> spirvCode( result.begin(), result.end() );
    outputFile.write( reinterpret_cast<const char *>( spirvCode.data() ),
                      spirvCode.size() * sizeof( uint32_t ) );
    outputFile.close();
    
    logOutput += "Compiled: " + filename + " -> " + outputPath + "\n";
    compiledCount++;
  }
  
  logOutput += "\n=== Compilation Summary ===\n";
  logOutput += "Successfully compiled: " + std::to_string( compiledCount ) + " shader(s)\n";
  if ( errorCount > 0 )
  {
    logOutput += "Errors: " + std::to_string( errorCount ) + " shader(s)\n";
  }
  
  return allSuccess;
}

bool reflectShader( const std::string & spirvPath, std::vector<DescriptorBinding> & bindings )
{
  bindings.clear();
  
  // Read SPIR-V file
  std::ifstream spirvFile( spirvPath, std::ios::binary );
  if ( !spirvFile.is_open() )
  {
    std::println( "Error: Could not open SPIR-V file: {}", spirvPath );
    return false;
  }
  
  std::vector<char> spirvCode( ( std::istreambuf_iterator<char>( spirvFile ) ),
                                std::istreambuf_iterator<char>() );
  spirvFile.close();
  
  // Create reflection module
  SpvReflectShaderModule module;
  SpvReflectResult result = spvReflectCreateShaderModule( 
    spirvCode.size(), spirvCode.data(), &module );
  
  if ( result != SPV_REFLECT_RESULT_SUCCESS )
  {
    std::println( "Error: Failed to create reflection module for: {}", spirvPath );
    return false;
  }
  
  // Enumerate descriptor bindings
  uint32_t descriptorSetCount = 0;
  result = spvReflectEnumerateDescriptorSets( &module, &descriptorSetCount, nullptr );
  if ( result != SPV_REFLECT_RESULT_SUCCESS )
  {
    spvReflectDestroyShaderModule( &module );
    return false;
  }
  
  if ( descriptorSetCount == 0 )
  {
    // No descriptor sets in this shader
    spvReflectDestroyShaderModule( &module );
    return true;
  }
  
  std::vector<SpvReflectDescriptorSet *> descriptorSets( descriptorSetCount );
  result = spvReflectEnumerateDescriptorSets( &module, &descriptorSetCount, descriptorSets.data() );
  if ( result != SPV_REFLECT_RESULT_SUCCESS )
  {
    spvReflectDestroyShaderModule( &module );
    return false;
  }
  
  // Extract binding information
  for ( uint32_t i = 0; i < descriptorSetCount; ++i )
  {
    const SpvReflectDescriptorSet * set = descriptorSets[i];
    
    for ( uint32_t j = 0; j < set->binding_count; ++j )
    {
      const SpvReflectDescriptorBinding * binding = set->bindings[j];
      
      DescriptorBinding desc;
      desc.set = set->set;
      desc.binding = binding->binding;
      desc.name = binding->name ? binding->name : "unnamed";
      
      // Determine type
      switch ( binding->descriptor_type )
      {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
          desc.type = "Sampler";
          break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
          desc.type = "Combined Image Sampler";
          break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
          desc.type = "Sampled Image";
          break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
          desc.type = "Storage Image";
          break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
          desc.type = "Uniform Buffer";
          break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
          desc.type = "Storage Buffer";
          break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
          desc.type = "Uniform Buffer Dynamic";
          break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
          desc.type = "Storage Buffer Dynamic";
          break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
          desc.type = "Input Attachment";
          break;
        default:
          desc.type = "Unknown";
          break;
      }
      
      bindings.push_back( desc );
    }
  }
  
  spvReflectDestroyShaderModule( &module );
  return true;
}

void renderMainLoopWindow( MainLoopState & mainLoopState, ResourceManagerState & resourceState )
{
  ImGui::Begin( "Main Loop" );
  
  // Compile button
  if ( ImGui::Button( "Compile All Shaders", ImVec2( 150, 30 ) ) )
  {
    mainLoopState.showCompileLog = true;
    compileAllShaders( mainLoopState.compileLog );
  }
  
  ImGui::SameLine();
  if ( ImGui::Button( "Add Pipeline", ImVec2( 120, 30 ) ) )
  {
    // Add a new pipeline from resources
    ImGui::OpenPopup( "SelectPipelineToAdd" );
  }
  
  // Popup to select which pipeline to add
  if ( ImGui::BeginPopup( "SelectPipelineToAdd" ) )
  {
    ImGui::Text( "Select Pipeline to Add" );
    ImGui::Separator();
    
    for ( const auto & resource : resourceState.resources )
    {
      if ( resource.type == ResourceType::Pipeline )
      {
        if ( ImGui::Selectable( resource.name.c_str() ) )
        {
          // Check if pipeline already exists
          bool exists = false;
          for ( const auto & p : mainLoopState.pipelines )
          {
            if ( p.pipelineName == resource.name )
            {
              exists = true;
              break;
            }
          }
          
          if ( !exists )
          {
            PipelineConfig newPipeline;
            newPipeline.pipelineName = resource.name;
            mainLoopState.pipelines.push_back( newPipeline );
          }
          
          ImGui::CloseCurrentPopup();
        }
      }
    }
    
    ImGui::EndPopup();
  }
  
  ImGui::Separator();
  
  // Display pipelines
  for ( size_t i = 0; i < mainLoopState.pipelines.size(); ++i )
  {
    auto & pipeline = mainLoopState.pipelines[i];
    
    ImGui::PushID( static_cast<int>( i ) );
    
    // Pipeline header
    bool isExpanded = ImGui::CollapsingHeader( pipeline.pipelineName.c_str(), ImGuiTreeNodeFlags_DefaultOpen );
    
    if ( isExpanded )
    {
      ImGui::Indent();
      
      // Vertex shader selection
      ImGui::Text( "Vertex Shader:" );
      ImGui::SameLine( 150 );
      
      std::string vertexLabel = pipeline.vertexShader.isAssigned ? 
        pipeline.vertexShader.shaderName : "None";
      
      if ( ImGui::BeginCombo( "##VertexShader", vertexLabel.c_str() ) )
      {
        if ( ImGui::Selectable( "None", !pipeline.vertexShader.isAssigned ) )
        {
          pipeline.vertexShader.isAssigned = false;
          pipeline.vertexShader.shaderName = "";
          pipeline.vertexShader.shaderPath = "";
          pipeline.descriptorBindings.clear();
        }
        
        // List all .vert shaders from compiled directory
        if ( std::filesystem::exists( "./compiled" ) )
        {
          for ( const auto & entry : std::filesystem::directory_iterator( "./compiled" ) )
          {
            std::string filename = entry.path().filename().string();
            if ( filename.find( ".vert.spv" ) != std::string::npos )
            {
              std::string shaderName = filename.substr( 0, filename.find( ".vert.spv" ) );
              bool isSelected = pipeline.vertexShader.isAssigned && 
                               pipeline.vertexShader.shaderName == shaderName;
              
              if ( ImGui::Selectable( shaderName.c_str(), isSelected ) )
              {
                pipeline.vertexShader.isAssigned = true;
                pipeline.vertexShader.shaderName = shaderName;
                pipeline.vertexShader.shaderPath = entry.path().string();
                
                // Reflect and update descriptor bindings
                std::vector<DescriptorBinding> bindings;
                if ( reflectShader( pipeline.vertexShader.shaderPath, bindings ) )
                {
                  pipeline.descriptorBindings = bindings;
                }
              }
            }
          }
        }
        
        ImGui::EndCombo();
      }
      
      // Fragment shader selection
      ImGui::Text( "Fragment Shader:" );
      ImGui::SameLine( 150 );
      
      std::string fragmentLabel = pipeline.fragmentShader.isAssigned ? 
        pipeline.fragmentShader.shaderName : "None";
      
      if ( ImGui::BeginCombo( "##FragmentShader", fragmentLabel.c_str() ) )
      {
        if ( ImGui::Selectable( "None", !pipeline.fragmentShader.isAssigned ) )
        {
          pipeline.fragmentShader.isAssigned = false;
          pipeline.fragmentShader.shaderName = "";
          pipeline.fragmentShader.shaderPath = "";
        }
        
        // List all .frag shaders from compiled directory
        if ( std::filesystem::exists( "./compiled" ) )
        {
          for ( const auto & entry : std::filesystem::directory_iterator( "./compiled" ) )
          {
            std::string filename = entry.path().filename().string();
            if ( filename.find( ".frag.spv" ) != std::string::npos )
            {
              std::string shaderName = filename.substr( 0, filename.find( ".frag.spv" ) );
              bool isSelected = pipeline.fragmentShader.isAssigned && 
                               pipeline.fragmentShader.shaderName == shaderName;
              
              if ( ImGui::Selectable( shaderName.c_str(), isSelected ) )
              {
                pipeline.fragmentShader.isAssigned = true;
                pipeline.fragmentShader.shaderName = shaderName;
                pipeline.fragmentShader.shaderPath = entry.path().string();
                
                // Merge fragment shader bindings with existing ones
                std::vector<DescriptorBinding> bindings;
                if ( reflectShader( pipeline.fragmentShader.shaderPath, bindings ) )
                {
                  // Merge with existing bindings
                  for ( const auto & newBinding : bindings )
                  {
                    bool found = false;
                    for ( auto & existingBinding : pipeline.descriptorBindings )
                    {
                      if ( existingBinding.set == newBinding.set && 
                           existingBinding.binding == newBinding.binding )
                      {
                        found = true;
                        break;
                      }
                    }
                    if ( !found )
                    {
                      pipeline.descriptorBindings.push_back( newBinding );
                    }
                  }
                }
              }
            }
          }
        }
        
        ImGui::EndCombo();
      }
      
      // Compute shader selection
      ImGui::Text( "Compute Shader:" );
      ImGui::SameLine( 150 );
      
      std::string computeLabel = pipeline.computeShader.isAssigned ? 
        pipeline.computeShader.shaderName : "None";
      
      if ( ImGui::BeginCombo( "##ComputeShader", computeLabel.c_str() ) )
      {
        if ( ImGui::Selectable( "None", !pipeline.computeShader.isAssigned ) )
        {
          pipeline.computeShader.isAssigned = false;
          pipeline.computeShader.shaderName = "";
          pipeline.computeShader.shaderPath = "";
        }
        
        // List all .comp shaders from compiled directory
        if ( std::filesystem::exists( "./compiled" ) )
        {
          for ( const auto & entry : std::filesystem::directory_iterator( "./compiled" ) )
          {
            std::string filename = entry.path().filename().string();
            if ( filename.find( ".comp.spv" ) != std::string::npos )
            {
              std::string shaderName = filename.substr( 0, filename.find( ".comp.spv" ) );
              bool isSelected = pipeline.computeShader.isAssigned && 
                               pipeline.computeShader.shaderName == shaderName;
              
              if ( ImGui::Selectable( shaderName.c_str(), isSelected ) )
              {
                pipeline.computeShader.isAssigned = true;
                pipeline.computeShader.shaderName = shaderName;
                pipeline.computeShader.shaderPath = entry.path().string();
                
                // Reflect and update descriptor bindings for compute
                std::vector<DescriptorBinding> bindings;
                if ( reflectShader( pipeline.computeShader.shaderPath, bindings ) )
                {
                  pipeline.descriptorBindings = bindings;
                }
              }
            }
          }
        }
        
        ImGui::EndCombo();
      }
      
      ImGui::Spacing();
      ImGui::Separator();
      
      // Descriptor bindings
      if ( !pipeline.descriptorBindings.empty() )
      {
        ImGui::Text( "Descriptor Bindings:" );
        
        if ( ImGui::BeginTable( "DescriptorBindings", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg ) )
        {
          ImGui::TableSetupColumn( "Set" );
          ImGui::TableSetupColumn( "Binding" );
          ImGui::TableSetupColumn( "Name" );
          ImGui::TableSetupColumn( "Type" );
          ImGui::TableSetupColumn( "Assigned Resource" );
          ImGui::TableHeadersRow();
          
          for ( size_t j = 0; j < pipeline.descriptorBindings.size(); ++j )
          {
            auto & binding = pipeline.descriptorBindings[j];
            
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex( 0 );
            ImGui::Text( "%u", binding.set );
            
            ImGui::TableSetColumnIndex( 1 );
            ImGui::Text( "%u", binding.binding );
            
            ImGui::TableSetColumnIndex( 2 );
            ImGui::Text( "%s", binding.name.c_str() );
            
            ImGui::TableSetColumnIndex( 3 );
            ImGui::Text( "%s", binding.type.c_str() );
            
            ImGui::TableSetColumnIndex( 4 );
            ImGui::PushID( static_cast<int>( j ) );
            
            std::string resourceLabel = binding.assignedResource.empty() ? 
              "None" : binding.assignedResource;
            
            if ( ImGui::BeginCombo( "##Resource", resourceLabel.c_str() ) )
            {
              if ( ImGui::Selectable( "None", binding.assignedResource.empty() ) )
              {
                binding.assignedResource = "";
              }
              
              // List compatible resources based on binding type
              for ( const auto & resource : resourceState.resources )
              {
                bool compatible = false;
                
                // Check compatibility
                if ( binding.type.find( "Buffer" ) != std::string::npos && 
                     resource.type == ResourceType::Buffer )
                {
                  compatible = true;
                }
                else if ( binding.type.find( "Sampler" ) != std::string::npos && 
                         resource.type == ResourceType::Sampler )
                {
                  compatible = true;
                }
                else if ( binding.type.find( "Image" ) != std::string::npos && 
                         resource.type == ResourceType::Texture )
                {
                  compatible = true;
                }
                
                if ( compatible )
                {
                  bool isSelected = binding.assignedResource == resource.name;
                  if ( ImGui::Selectable( resource.name.c_str(), isSelected ) )
                  {
                    binding.assignedResource = resource.name;
                  }
                }
              }
              
              ImGui::EndCombo();
            }
            
            ImGui::PopID();
          }
          
          ImGui::EndTable();
        }
      }
      else
      {
        ImGui::TextDisabled( "No descriptor bindings (assign shaders to see bindings)" );
      }
      
      ImGui::Unindent();
      ImGui::Spacing();
    }
    
    ImGui::PopID();
  }
  
  ImGui::End();
  
  // Compile log window
  if ( mainLoopState.showCompileLog )
  {
    ImGui::Begin( "Compile Log", &mainLoopState.showCompileLog );
    ImGui::TextUnformatted( mainLoopState.compileLog.c_str() );
    ImGui::End();
  }
}

} // namespace ui
