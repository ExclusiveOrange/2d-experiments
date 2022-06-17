#pragma once

#include <optional>

#include <glm/geometric.hpp>

#include "../Intersection.hpp"
#include "../Ray.hpp"

namespace raycasting::shapes
{
  namespace detail
  {
    class Quad
    {
      const glm::vec3 center, n, a, b;
      const float al2, bl2;
    public:
      Quad(glm::vec3 center, glm::vec3 a, glm::vec3 b)
        : center{center}, n{glm::normalize(glm::cross(b, a))}, a{a}, b{b}
        , al2{glm::dot(a, a)}, bl2{glm::dot(b, b)} {}

      [[nodiscard]]
      std::optional<Intersection>
      intersect(Ray ray) const
      {
        // ray: o + t*d
        // plane: (x - center) dot n = 0
        // intersection:
        //   ([o + t*d] - center) dot n = 0
        //   (o - center) dot n + t * d dot n = 0
        //   t = [(center - o) dot n] / (d dot n)

        constexpr const float small = 0.001f;

        const float dDotN = glm::dot(ray.direction, n);

        if (glm::abs(dDotN) < small)
          return std::nullopt; // ray parallel or close to parallel with plane

        const float t = glm::dot(center - ray.origin, n) / dDotN;

        const glm::vec3 isect{ray.origin + t * ray.direction};

        if (glm::abs(glm::dot(isect - center, a)) > al2 || glm::abs(glm::dot(isect - center, b)) > bl2)
          return std::nullopt; // ray intersects plane but not within bounds of quad

        return Intersection{.position = isect, .normal = n, .distance = t};
      }
    };
  }

  static
  std::function<std::optional<Intersection>(Ray ray)>
  makeQuad(glm::vec3 diffuse, glm::vec3 center, glm::vec3 a, glm::vec3 b)
  {
    return [=, quad = detail::Quad(center, a, b)](Ray ray)
    {
      auto i = quad.intersect(ray);
      return i ? (i->diffuse = diffuse, i) : i;
    };
  }

  static
  std::function<std::optional<Intersection>(Ray ray)>
  makeQuad(const std::function<glm::vec3(glm::vec3)> &xyzToDiffuse, glm::vec3 center, glm::vec3 a, glm::vec3 b)
  {
    return [=, quad = detail::Quad(center, a, b)](Ray ray)
    {
      auto i = quad.intersect(ray);
      return i ? (i->diffuse = xyzToDiffuse(i->position), i) : i;
    };
  }
}