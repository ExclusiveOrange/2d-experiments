#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "../Intersection.hpp"
#include "../Ray.hpp"

namespace raycasting::csg
{
  [[nodiscard]]
  static
  std::function<std::optional<Intersection>(Ray ray)>
  makeUnion(std::vector<std::function<std::optional<Intersection>(Ray ray)>> intersectors)
  {
    return [intersectors = std::move(intersectors)](Ray ray)
    {
      std::optional<Intersection> intersection{};

      for (const auto &intersector: intersectors)
        if (std::optional<Intersection> thisIntersection = intersector(ray))
          if (!intersection || intersection->distance > thisIntersection->distance)
            intersection = thisIntersection;

      return intersection;
    };
  }
}
