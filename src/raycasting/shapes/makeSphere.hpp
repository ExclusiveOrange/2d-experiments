#pragma once

#include <functional>
#include <optional>

#include <glm/geometric.hpp>

#include "../Intersection.hpp"
#include "../Ray.hpp"

namespace raycasting::shapes
{
  static
  std::function<std::optional<Intersection>(Ray ray)>
  makeSphere(glm::vec3 diffuse, glm::vec3 center, float radius)
  {
    const float sqradius = radius * radius;

    return [=](Ray ray) -> std::optional<Intersection>
    {
      // ray is ray.origin + d * ray.unitDirection = x
      // sphere is |x - center| = radius
      // then intersections satisfy |(ray.origin + d * ray.unitDirection) - center| = radius
      // solve for d

      auto square = [](auto x) {return x * x;};

      const glm::vec3 originMinusCenter = ray.origin - center;
      const float ray_dot_originMinusCenter = glm::dot(ray.direction, originMinusCenter);

      float insideRadical = square(ray_dot_originMinusCenter) - glm::dot(originMinusCenter, originMinusCenter) + sqradius;

      if (insideRadical <= 0) // no intersection or exactly one glancing intersection (which I ignore)
        return std::nullopt;

      // two intersections; want to pick most backward intersection
      float sqrtInsideRadical = glm::sqrt(insideRadical);
      float outsideRadical = -ray_dot_originMinusCenter;
      float d = outsideRadical - sqrtInsideRadical;

      // sphere is wholly ahead of ray origin and emits a surface intersection
      Intersection intersection{.position = ray.origin + d * ray.direction, .diffuse = diffuse, .distance = d};
      intersection.normal = glm::normalize(intersection.position - center);
      return intersection;
    };
  }

  static
  std::function<std::optional<Intersection>(Ray ray)>
  makeSphere(const std::function<glm::vec3(glm::vec3)> &xyzToDiffuse, glm::vec3 center, float radius)
  {
    const float sqradius = radius * radius;

    return [=](Ray ray) -> std::optional<Intersection>
    {
      // ray is ray.origin + d * ray.unitDirection = x
      // sphere is |x - center| = radius
      // then intersections satisfy |(ray.origin + d * ray.unitDirection) - center| = radius
      // solve for d

      const glm::vec3 originMinusCenter = ray.origin - center;
      const float ray_dot_originMinusCenter = glm::dot(ray.direction, originMinusCenter);

      float insideRadical = ray_dot_originMinusCenter * ray_dot_originMinusCenter - glm::dot(originMinusCenter, originMinusCenter) + sqradius;

      if (insideRadical <= 0) // no intersection or exactly one glancing intersection (which I ignore)
        return std::nullopt;

      // two intersections; want to pick most backward intersection
      float sqrtInsideRadical = glm::sqrt(insideRadical);
      float outsideRadical = -ray_dot_originMinusCenter;
      float d = outsideRadical - sqrtInsideRadical;

      // sphere is wholly ahead of ray origin and emits a surface intersection
      Intersection intersection{.position = ray.origin + d * ray.direction, .distance = d};
      intersection.normal = glm::normalize(intersection.position - center);
      intersection.diffuse = xyzToDiffuse(intersection.position);
      return intersection;
    };
  }
}