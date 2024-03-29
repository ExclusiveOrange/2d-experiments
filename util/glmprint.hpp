#pragma once

#include <ostream>

#include <glm/vec3.hpp>

static
std::ostream &
operator <<(std::ostream &os, const glm::vec3 v)
{
  return os << "<" << v.x << ", " << v.y << ", " << v.z << ">";
}
