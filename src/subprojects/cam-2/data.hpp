#pragma once
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace data
{
  inline constexpr std::string_view AppName    = "MyApp";
  inline constexpr std::string_view EngineName = "MyEngine";

  struct PushConstants
  {
    glm::mat4 view;
    glm::mat4 proj;
  };

  struct Vertex
  {
    glm::vec2 position;
    glm::vec3 color;
  };

  struct InstanceData
  {
    glm::vec3 position;
  };

  inline constexpr float side   = 1.0f;
  inline constexpr float height = side * 0.86602540378f;  // sqrt(3)/2

  inline constexpr std::array<Vertex, 3> triangleVertices = {
    Vertex{ glm::vec2( 0.0f, -height / 3.0f ), glm::vec3( 1.0f, 0.5f, 0.5f ) },         // bottom
    Vertex{ glm::vec2( 0.5f, height * 2.0f / 3.0f ), glm::vec3( 0.5f, 1.0f, 0.5f ) },   // right
    Vertex{ glm::vec2( -0.5f, height * 2.0f / 3.0f ), glm::vec3( 0.5f, 0.5f, 1.0f ) },  // left
  };

  inline constexpr int    gridMin       = -20;
  inline constexpr int    gridMax       = 20;
  inline constexpr int    gridCount     = gridMax - gridMin + 1;
  inline constexpr size_t instanceCount = static_cast<size_t>( gridCount ) * gridCount * gridCount;

  // Generate instances at compile time
  inline constexpr auto createInstances()
  {
    std::array<InstanceData, instanceCount> instancesPos{};
    size_t                                  index = 0;

    for ( int x = gridMin; x <= gridMax; ++x )
    {
      for ( int y = gridMin; y <= gridMax; ++y )
      {
        for ( int z = gridMin; z <= gridMax; ++z )
        {
          instancesPos[index++] = InstanceData{ glm::vec3( float( x ) * 3.0f, float( y ) * 3.0f, float( z ) * 3.0f ) };
        }
      }
    }

    return instancesPos;
  }

  inline constexpr std::array<InstanceData, instanceCount> instancesPos = createInstances();

}  // namespace data
