#pragma once

#include <functional>
#include <optional>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include "../DepthIntersection.hpp"
#include "../Ray.hpp"

namespace raycasting::volumes
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
      std::optional<DepthIntersection>
      intersect(Ray ray) const
      {
        const glm::vec3 originMinusCenter = ray.origin - center;
        const float ray_dot_originMinusCenter = glm::dot(ray.d, originMinusCenter);

        float insideRadical = sq(ray_dot_originMinusCenter) - glm::dot(originMinusCenter, originMinusCenter) + sqradius;

        if (insideRadical <= 0) // no intersection or exactly one glancing intersection (which I ignore)
          return std::nullopt;

        float sqrtInsideRadical = glm::sqrt(insideRadical);
        float outsideRadical = -ray_dot_originMinusCenter;
        float d0 = outsideRadical - sqrtInsideRadical;
        float d1 = outsideRadical + sqrtInsideRadical;

        return DepthIntersection{
          .distance0 = outsideRadical - sqrtInsideRadical,
          .distance1 = outsideRadical + sqrtInsideRadical};
      }
    };
  }

  [[nodiscard]]
  static
  std::function<std::optional<DepthIntersection>(Ray)>
  makeSphere(glm::vec3 center, float radius)
  {
    return [sphere = detail::Sphere{center, radius}](Ray ray) -> std::optional<DepthIntersection>
    {
      return sphere.intersect(ray);
    };
  }
}

