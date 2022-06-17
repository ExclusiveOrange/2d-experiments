#pragma once

#include <functional>
#include <optional>
#include <stdexcept>

#include <glm/vec2.hpp>
#include <glm/geometric.hpp>

#include "../Intersection.hpp"
#include "../Ray.hpp"

namespace raycasting::shapes
{
  namespace detail
  {

    class Cone
    {
      float sqradius, minz, maxz, zslope;

      static auto sq(auto x) {return x * x;}

    public:
      Cone(float height, float radiusAtHeight)
      {
        if (height == 0)
          throw std::runtime_error("makeCone: height cannot be 0");

        if (radiusAtHeight <= 0)
          throw std::runtime_error("makecone: radiusAtHeight must be > 0");

        sqradius = sq(radiusAtHeight / height);
        minz = height < 0 ? height : 0.f;
        maxz = height < 0 ? 0.f : height;
        zslope = -radiusAtHeight / height;
      }

      [[nodiscard]]
      std::optional<Intersection>
      intersect(Ray ray) const
      {
        // ray is o + t * d
        //   where o is the ray origin and d is the ray direction
        // cone is |xy| = r*z
        //   where r is radiusAt_ZOne
        // then substitute ray for xyz and solve for t using quadratic formula
        //   t = (-b +- sqrt(b^2 - 4ac)) / (2a)
        // where:
        //   2a = 2(dx^2 + dy^2 - r^2*dz^2)
        //   4ac = 2a*2c
        //   2c = 2(ox^2 + oy^2 - r^2*oz^2)
        //   b = 2(dx + dy - r^2*oz*dz)
        // in quadratic formula:
        //   if a = 0 or is very small then can't continue
        //   if b^2 - 4ac is < 0 then can't continue (b^2 < 4ac)

        constexpr float small = 0.0005f;

        const float a = sq(ray.d.x) + sq(ray.d.y) - sqradius * sq(ray.d.z);

        if (glm::abs(a) < small)
          return std::nullopt; // denominator too small for reliable calculation

        const float two_a = a + a;
        const float b = 2.f * (ray.o.x * ray.d.x + ray.o.y * ray.d.y - sqradius * ray.o.z * ray.d.z);
        const float c = sq(ray.o.x) + sq(ray.o.y) - sqradius * sq(ray.o.z);
        const float bb = sq(b);
        const float four_ac = two_a * (c + c);
        const float bb_minus_four_ac = bb - four_ac;

        // TODO: there is another case: ray is parallel with surface of cone

        if (bb_minus_four_ac < 0)
          return std::nullopt; // no intersections

        float t0 = (-b - glm::sqrt(bb - four_ac)) / two_a;
        float t1 = (-b + glm::sqrt(bb - four_ac)) / two_a;

        if (t0 > t1)
          std::swap(t0, t1);

        const glm::vec3 isect0 = ray.o + t0 * ray.d;
        const glm::vec3 isect1 = ray.o + t1 * ray.d;

        std::optional<std::pair<glm::vec3, float>> isect_and_d{};

        if (isect0.z >= minz && isect0.z <= maxz)
          isect_and_d = {isect0, t0};
        else if (isect1.z >= minz && isect1.z <= maxz)
          isect_and_d = {isect1, t1};
        else
          return std::nullopt; // both intersections are out of bounds on the z axis

        return Intersection{
          .position = isect_and_d->first,
          .normal = glm::normalize(glm::vec3(glm::normalize(glm::vec2(isect_and_d->first)), zslope)),
          .distance = isect_and_d->second};
      }
    };
  }

  [[nodiscard]]
  static
  std::function<std::optional<Intersection>(Ray ray)>
  makeCone(glm::vec3 diffuse, float height, float radiusAtHeight) // height can be negative
  {
    detail::Cone cone{height, radiusAtHeight};

    return [=](Ray ray) -> std::optional<Intersection>
    {
      auto maybeIntersection = cone.intersect(ray);

      if (maybeIntersection)
        maybeIntersection->diffuse = diffuse;

      return maybeIntersection;
    };
  }

  [[nodiscard]]
  static
  std::function<std::optional<Intersection>(Ray ray)>
  makeCone(const std::function<glm::vec3(glm::vec3)> &xyzToDiffuse, float height, float radiusAtHeight) // height can be negative
  {
    detail::Cone cone{height, radiusAtHeight};

    return [=](Ray ray) -> std::optional<Intersection>
    {
      auto maybeIntersection = cone.intersect(ray);

      if (maybeIntersection)
        maybeIntersection->diffuse = xyzToDiffuse(maybeIntersection->position);

      return maybeIntersection;
    };
  }
}