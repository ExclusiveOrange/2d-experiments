#pragma once

#include <functional>
#include <optional>

#include <glm/geometric.hpp>

#include "../Intersection.hpp"
#include "../Ray.hpp"

namespace raycasting::shapes
{
  namespace detail
  {
    class Sphere
    {
      const glm::vec3 center;
      const float sqradius;

      static auto sq(auto x) {return x * x;}

    public:
      Sphere(glm::vec3 center, float radius)
        : center{center}, sqradius{sq(radius)} {}

      [[nodiscard]]
      std::optional<Intersection>
      intersect(Ray ray) const
      {
        // ray is ray.origin + d * ray.unitDirection = x
        // sphere is |x - center| = radius
        // then intersections satisfy |(ray.origin + d * ray.unitDirection) - center| = radius
        // solve for d

        const glm::vec3 originMinusCenter = ray.origin - center;
        const float ray_dot_originMinusCenter = glm::dot(ray.d, originMinusCenter);

        float insideRadical = sq(ray_dot_originMinusCenter) - glm::dot(originMinusCenter, originMinusCenter) + sqradius;

        if (insideRadical <= 0) // no intersection or exactly one glancing intersection (which I ignore)
          return std::nullopt;

        // two intersections; want to pick most backward intersection
        float sqrtInsideRadical = glm::sqrt(insideRadical);
        float outsideRadical = -ray_dot_originMinusCenter;
        float d = outsideRadical - sqrtInsideRadical;

        // sphere is wholly ahead of ray origin and emits a surface intersection
        Intersection intersection{.position = ray.origin + d * ray.direction, .distance = d};
        intersection.normal = glm::normalize(intersection.position - center);
        return intersection;
      }
    };
  }

  [[nodiscard]]
  static
  std::function<std::optional<Intersection>(Ray ray)>
  makeSphere(glm::vec3 diffuse, glm::vec3 center, float radius)
  {
    return [=, sphere = detail::Sphere{center, radius}](Ray ray) -> std::optional<Intersection>
    {
      auto i = sphere.intersect(ray);
      return i ? (i->diffuse = diffuse, i) : i;
    };
  }

  [[nodiscard]]
  static
  std::function<std::optional<Intersection>(Ray ray)>
  makeSphere(const std::function<glm::vec3(glm::vec3)> &xyzToDiffuse, glm::vec3 center, float radius)
  {
    return [=, sphere = detail::Sphere{center, radius}](Ray ray) -> std::optional<Intersection>
    {
      auto i = sphere.intersect(ray);
      return i ? (i->diffuse = xyzToDiffuse(i->position), i) : i;
    };
  }
}