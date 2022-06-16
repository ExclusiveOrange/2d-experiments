#pragma once

#include <glm/vec3.hpp>

namespace raycasting
{
  struct Ray
  {
    glm::vec3 origin, unitDirection;
  };
}