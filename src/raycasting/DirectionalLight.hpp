#pragma once

#include "glm/geometric.hpp"
#include <glm/vec3.hpp>

namespace raycasting
{
  struct DirectionalLight
  {
    DirectionalLight(glm::vec3 direction, glm::vec3 intensity)
      : ndirection{-direction}, intensity{intensity} {}

    [[nodiscard]] glm::vec3
    calculate(glm::vec3 position, glm::vec3 normal) const
    {
      return intensity * glm::dot(normal, ndirection);
    }

  private:
    const glm::vec3 ndirection, intensity;
  };
}
