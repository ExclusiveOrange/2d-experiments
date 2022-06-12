#pragma once

#include "raycasting.hpp"

// TODO: delete
#include <iostream>

namespace raycasting::shapes
{
  // TODO: make some kind of glm helper header with this in it
  std::ostream &operator<<(std::ostream &os, const glm::vec3 v)
  {
    os << "<" << v.x << ", " << v.y << ", " << v.z << ">";
    return os;
  }

  struct QuadUp : Intersectable
  {
    QuadUp(glm::vec3 center, float radius)
      : center{center}, radius{radius} {}

    std::optional<Intersection>
    intersect(const Ray &ray) const override
    {
      constexpr const float small = 0.001f;
      // ray: o + t*d
      // plane: (x - p) dot n = 0
      // intersection:
      //   ([o + t*d] - p) dot n = 0
      //   (o - p) dot n + t * d dot n = 0
      //   t = [(p - o) dot n] / (d dot n)
      // where in this case n = raycasting::up and p = <0,0,0>

      const float dDotN = glm::dot(ray.unitDirection, up);

      if (-small < dDotN && dDotN < small) // ray parallel or close to parallel with plane
        return std::nullopt;

      const float t = glm::dot(-ray.origin, up) / dDotN;
      const glm::vec3 isect{ray.origin + t * ray.unitDirection};

      if (glm::abs(glm::dot(isect - center, right)) > radius || glm::abs(glm::dot(isect - center, forward)) > radius)
        return std::nullopt;

      return Intersection{.position = isect, .normal = up, .distance = t};
    }

  private:
    const glm::vec3 center;
    const float radius;
  };

  struct Quad : Intersectable
  {
    Quad(glm::vec3 center, glm::vec3 a, glm::vec3 b)
      : p{center}, a{a}, b{b}, n{glm::normalize(glm::cross(a, b))}, al2{glm::dot(a, a)}, bl2{glm::dot(b, b)} {}

    std::optional<Intersection>
    intersect(const Ray &ray) const override
    {
      constexpr const float small = 0.001f;

      // ray: o + t*d
      // plane: (x - p) dot n = 0
      // intersection:
      //   ([o + t*d] - p) dot n = 0
      //   (o - p) dot n + t * d dot n = 0
      //   t = [(p - o) dot n] / (d dot n)
      // where in this case n = raycasting::up and p = <0,0,0>

      const float dDotN = glm::dot(ray.unitDirection, n);

      if (-small < dDotN && dDotN < small)
        return std::nullopt; // ray parallel or close to parallel with plane

      const float t = glm::dot(p - ray.origin, n) / dDotN;

      const glm::vec3 isect{ray.origin + t * ray.unitDirection};

      if (glm::abs(glm::dot(isect - p, a)) > al2 || glm::abs(glm::dot(isect - p, b)) > bl2)
        return std::nullopt; // ray intersects plane but not within bounds of quad

      return Intersection{.position = isect, .normal = up, .distance = t};
    }

  private:
    const glm::vec3 p, a, b, n;
    const float al2, bl2;
  };

  struct Sphere : Intersectable
  {
    Sphere(glm::vec3 center, float radius)
      : center{center}, sqradius{radius * radius} {}

    std::optional<Intersection>
    intersect(const Ray &ray) const override
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
      return intersection;
    }

  private:
    const glm::vec3 center;
    const float sqradius;
  };
}
