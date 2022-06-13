#pragma once

#include <ostream>

#include <glm/glm.hpp>

#include "glmprint.hpp"

struct MovementVectors
{
  const glm::vec3
    right, left, up, down,
    upright, downleft, upleft, downright;

  MovementVectors(glm::mat3 screenToWorld)
    : right{normalizedxy(glm::vec3{1.f, 0.f, 0.f} * screenToWorld)}
    , left{-right}
    , up{normalizedxy(glm::vec3{0.f, 1.f, 0.f} * screenToWorld)}
    , down{-up}
    , upright{normalizedxy(glm::vec3(1.f, 1.f, 0.f) * screenToWorld)}
    , downleft{-upright}
    , upleft{normalizedxy(glm::vec3(-1.f, 1.f, 0.f) * screenToWorld)}
    , downright{-upleft} {}

  friend std::ostream &operator<<(std::ostream &os, const MovementVectors &m)
  {
    return os
      << "left(" << m.left << "), right(" << m.right << ")\n"
      << "down(" << m.down << "), up(" << m.up << ")\n"
      << "downleft(" << m.downleft << "), downright(" << m.downright << ")\n"
      << "upleft(" << m.upleft << "), upright(" << m.upright << ")\n";
  }

private:
  static glm::vec3 normalizedxy(glm::vec2 v) {return glm::vec3{glm::normalize(v), 0.f};}
};
