#pragma once

#include <optional>

#include <glm/vec3.hpp>

#include <function_traits.hpp>

#include "../../CpuImageWithDepth.hpp"

#include "../DirectionalLight.hpp"
#include "../Intersection.hpp"
#include "../Ray.hpp"

namespace raycasting::cameras
{
  // NOTE: shadows
  // Probably want to avoid shadows on pre-rendered objects for now in order to avoid
  // contradictions with varying lighting environments during run-time.
  // If any shadows are generated it should probably be as a separate image with translucency so that it can be moved relative to the caster.

  struct Orthogonal
  {
    // position is always at origin because rays are not limited to camera plane
    glm::vec3 normal;
    glm::vec3 xstep, ystep; // change in world coordinates from change in pixel coordinates

    // Orthogonal.render
    // Depth is centered at origin with uint8_t value 127.
    // Depth value 255 is reserved for rays which do not intersect anything or where the intersection distance is >= +128.0f.
    void render(
      const ViewOfCpuImageWithDepth &destImage,
      Function<std::optional<Intersection>(Ray ray)> auto &&intersect,
      const glm::vec3 minLight,
      const DirectionalLight *directionalLights,
      int numDirectionalLights,
      uint32_t defaultDrgb = 0xff000000)
    const
    {
      Ray ray{.direction = this->normal};

      for (int y = 0; y < destImage.h; ++y)
      {
        glm::vec3 yOffset = ((float)destImage.h * -0.5f + (float)y + 0.5f) * ystep;

        for (int x = 0; x < destImage.w; ++x)
        {
          glm::vec3 xOffset = ((float)destImage.w * -0.5f + (float)x + 0.5f) * xstep;
          ray.origin = yOffset + xOffset; // this camera's origin is always the world origin

          uint32_t drgb;

          if (std::optional<Intersection> i = intersect(ray))
          {
            glm::vec3 lightSum{};

            for (int il = 0; il < numDirectionalLights; ++il)
              lightSum += directionalLights[il].calculate(i->position, i->normal);

            lightSum = glm::clamp(lightSum, minLight, glm::vec3{1.f});
            glm::vec3 color = 255.f * glm::clamp(lightSum * i->diffuse, glm::vec3{0.f}, glm::vec3{1.f});

            auto depth = uint8_t(127.f + glm::clamp(i->distance, -127.f, 128.f));

            drgb = (depth << 24) | (uint32_t(color.x) << 16) | (uint32_t(color.y) << 8) | uint32_t(color.z);
          }
          else
            drgb = defaultDrgb;

          int dindex = y * destImage.w + x;
          destImage.drgb[dindex] = drgb;
        }
      }
    }
  };

}
