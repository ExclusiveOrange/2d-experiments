#pragma once

#include <functional>
#include <optional>

#include "../Intersection.hpp"
#include "../Ray.hpp"

namespace raycasting::transform
{
  std::function<std::optional<Intersection>(Ray)>
  translate(const std::function<std::optional<Intersection>(Ray)> &function, glm::vec3 offset)
  {
    return [=](Ray ray) -> std::optional<Intersection>
    {
      ray.o -= offset;
      if (std::optional<Intersection> maybeIntersection{function(ray)})
      {
        maybeIntersection->position += offset;
        return maybeIntersection;
      }
      else
        return std::nullopt;
    };
  }
}
