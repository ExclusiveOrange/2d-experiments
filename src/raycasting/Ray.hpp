#pragma once

#include <glm/vec3.hpp>

namespace raycasting
{
  struct Ray
  {
    union { glm::vec3 origin, o; };
    union { glm::vec3 direction, d; };
  };
}