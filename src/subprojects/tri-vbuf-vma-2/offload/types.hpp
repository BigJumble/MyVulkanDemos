#pragma once

#include <glm/glm.hpp>

namespace offload
{

  struct PushConstants
  {
    glm::vec2 pos;
  };

  struct Vertex
  {
    glm::vec2 position;
    glm::vec3 color;
  };

}  // namespace offload
