#pragma once

#include "raycasting.hpp"

namespace raycasting::shapes
{
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
      
      // two intersections; want to pick most backward intersection unless behind ray origin
      float sqrtInsideRadical = glm::sqrt(insideRadical);
      float outsideRadical = -ray_dot_originMinusCenter;
      
      // most forward intersection
      float d1 = outsideRadical + sqrtInsideRadical;
      if (d1 <= 0) // most-forward intersection is behind ray origin
        return std::nullopt;
      
      // else most forward intersection is ahead of ray origin,
      // and most backward intersection is behind or ahead of ray origin
      float d0 = outsideRadical - sqrtInsideRadical;
      
      // ray origin is inside sphere so clamp intersection to ray origin
      if (d0 < 0.f)
        return Intersection{.position = ray.origin, .normal = ray.unitDirection, .distance = 0.f};
      
      // sphere is wholly ahead of ray origin and emits a surface intersection
      Intersection intersection{.position = ray.origin + d0 * ray.unitDirection, .distance = d0};
      intersection.normal = glm::normalize(intersection.position - center);
      return intersection;
    }
  
  private:
    const glm::vec3 center;
    const float sqradius;
  };
}
