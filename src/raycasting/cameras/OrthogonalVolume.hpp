#pragma once

#include <optional>

#include <glm/common.hpp>
#include <glm/vec3.hpp>

#include <function_traits.hpp>

#include "../../CpuDepthVolume.hpp"

#include "../DepthIntersection.hpp"
#include "../Ray.hpp"

namespace raycasting::cameras
{
  // For generating CpuDepthVolume images.

  struct OrthogonalVolume
  {
    // position is always at origin because rays are not limited to camera plane
    glm::vec3 normal;
    glm::vec3 xstep, ystep; // change in world coordinates from change in pixel coordinates

    void
    render(
      const ViewOfCpuDepthVolume &destVolume,
      Function<std::optional<DepthIntersection>(Ray ray)> auto &&intersect)
    const
    {
      Ray ray{.direction = normal};

      for (int y = 0; y < destVolume.h; ++y)
      {
        glm::vec3 yOffset = ((float)destVolume.h * -0.5f + (float)y + 0.5f) * ystep;
        for (int x = 0; x < destVolume.w; ++x)
        {
          glm::vec3 xOffset = ((float)destVolume.w * -0.5f + (float)x + 0.5f) * xstep;
          ray.origin = yOffset + xOffset; // this camera's origin is always the world origin

          uint16_t depthAndThickness;

          if (std::optional<DepthIntersection> i = intersect(ray))
          {
            auto thickness = uint8_t(glm::clamp(i->distance1 - i->distance0, 0.f, 255.f));
            auto depth = uint8_t(127.f + glm::clamp(i->distance0, -127.f, 128.f));

            depthAndThickness = uint16_t(thickness) << 8 | depth;
          }
          else
            depthAndThickness = 0;

          int dindex = y * destVolume.w + x;
          destVolume.depthAndThickness[dindex] = depthAndThickness;
        }
      }
    }
  };
}