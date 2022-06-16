#pragma once

#include <glm/vec3.hpp>

namespace directions
{
  constexpr const glm::vec3 right{1.f, 0.f, 0.f};
  constexpr const glm::vec3 left = -right;

  constexpr const glm::vec3 forward{0.f, 1.f, 0.f};
  constexpr const glm::vec3 backward = -forward;

  constexpr const glm::vec3 up{0.f, 0.f, 1.f};
  constexpr const glm::vec3 down = -up;
}
