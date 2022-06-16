#pragma once

#include <glm/vec3.hpp>

namespace raycasting
{
  struct Intersection
  {
    glm::vec3 position, normal;
    glm::vec3 diffuse;
    float distance;
  };
}
