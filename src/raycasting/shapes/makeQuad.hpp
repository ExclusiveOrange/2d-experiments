#pragma once

#include <optional>

#include <glm/geometric.hpp>

#include "../Intersection.hpp"
#include "../Ray.hpp"

namespace raycasting::shapes
{
  static
  std::function<std::optional<Intersection>(Ray ray)>
  makeQuad(glm::vec3 diffuse, glm::vec3 center, glm::vec3 a, glm::vec3 b)
  {
    const glm::vec3 n{glm::normalize(glm::cross(b, a))};
    const float al2{glm::dot(a, a)}, bl2{glm::dot(b, b)};

    return [=]
      (Ray ray) -> std::optional<Intersection>
    {
      constexpr const float small = 0.001f;

      // ray: o + t*d
      // plane: (x - center) dot n = 0
      // intersection:
      //   ([o + t*d] - center) dot n = 0
      //   (o - center) dot n + t * d dot n = 0
      //   t = [(center - o) dot n] / (d dot n)

      const float dDotN = glm::dot(ray.unitDirection, n);

      if (glm::abs(dDotN) < small)
        return std::nullopt; // ray parallel or close to parallel with plane

      const float t = glm::dot(center - ray.origin, n) / dDotN;

      const glm::vec3 isect{ray.origin + t * ray.unitDirection};

      if (glm::abs(glm::dot(isect - center, a)) > al2 || glm::abs(glm::dot(isect - center, b)) > bl2)
        return std::nullopt; // ray intersects plane but not within bounds of quad

      return Intersection{.position = isect, .normal = n, .diffuse = diffuse, .distance = t};
    };
  }

  static
  std::function<std::optional<Intersection>(Ray ray)>
  makeQuad(const std::function<glm::vec3(glm::vec3)> &xyzToDiffuse, glm::vec3 center, glm::vec3 a, glm::vec3 b)
  {
    const glm::vec3 n{glm::normalize(glm::cross(b, a))};
    const float al2{glm::dot(a, a)}, bl2{glm::dot(b, b)};

    return [=]
      (Ray ray) -> std::optional<Intersection>
    {
      constexpr const float small = 0.001f;

      // ray: o + t*d
      // plane: (x - center) dot n = 0
      // intersection:
      //   ([o + t*d] - center) dot n = 0
      //   (o - center) dot n + t * d dot n = 0
      //   t = [(center - o) dot n] / (d dot n)

      const float dDotN = glm::dot(ray.unitDirection, n);

      if (glm::abs(dDotN) < small)
        return std::nullopt; // ray parallel or close to parallel with plane

      const float t = glm::dot(center - ray.origin, n) / dDotN;

      const glm::vec3 isect{ray.origin + t * ray.unitDirection};

      if (glm::abs(glm::dot(isect - center, a)) > al2 || glm::abs(glm::dot(isect - center, b)) > bl2)
        return std::nullopt; // ray intersects plane but not within bounds of quad

      return Intersection{.position = isect, .normal = n, .diffuse = xyzToDiffuse(isect), .distance = t};
    };
  }
}