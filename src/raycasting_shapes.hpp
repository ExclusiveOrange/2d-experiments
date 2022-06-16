#pragma once

#include "raycasting.hpp"

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
      const float ray_dot_originMinusCenter = glm::dot(ray.unitDirection, originMinusCenter);

      float insideRadical = square(ray_dot_originMinusCenter) - glm::dot(originMinusCenter, originMinusCenter) + sqradius;

      if (insideRadical <= 0) // no intersection or exactly one glancing intersection (which I ignore)
        return std::nullopt;

      // two intersections; want to pick most backward intersection
      float sqrtInsideRadical = glm::sqrt(insideRadical);
      float outsideRadical = -ray_dot_originMinusCenter;
      float d = outsideRadical - sqrtInsideRadical;

      // sphere is wholly ahead of ray origin and emits a surface intersection
      Intersection intersection{.position = ray.origin + d * ray.unitDirection, .diffuse = diffuse, .distance = d};
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

      auto square = [](auto x) {return x * x;};

      const glm::vec3 originMinusCenter = ray.origin - center;
      const float ray_dot_originMinusCenter = glm::dot(ray.unitDirection, originMinusCenter);

      float insideRadical = square(ray_dot_originMinusCenter) - glm::dot(originMinusCenter, originMinusCenter) + sqradius;

      if (insideRadical <= 0) // no intersection or exactly one glancing intersection (which I ignore)
        return std::nullopt;

      // two intersections; want to pick most backward intersection
      float sqrtInsideRadical = glm::sqrt(insideRadical);
      float outsideRadical = -ray_dot_originMinusCenter;
      float d = outsideRadical - sqrtInsideRadical;

      // sphere is wholly ahead of ray origin and emits a surface intersection
      Intersection intersection{.position = ray.origin + d * ray.unitDirection, .distance = d};
      intersection.normal = glm::normalize(intersection.position - center);
      intersection.diffuse = xyzToDiffuse(intersection.position);
      return intersection;
    };
  }
}
