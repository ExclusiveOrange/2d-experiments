#pragma once

#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

#include "gradient.hpp"

namespace noisyDiffuse
{
  std::function<glm::vec3(glm::vec3)>
  makeNoisyDiffuse(const std::function<glm::vec3(float t)> &gradientZeroToOne)
  {
    return [=](glm::vec3 x) -> glm::vec3 { return gradientZeroToOne(0.5f + 0.5f * glm::perlin(x)); };
  }
}

