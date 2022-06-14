#pragma once

#include <optional>

#include <glm/glm.hpp>

#include "function_traits.hpp"

namespace raycasting
{
  constexpr const glm::vec3 right{1.f, 0.f, 0.f};
  constexpr const glm::vec3 left = -right;

  constexpr const glm::vec3 forward{0.f, 1.f, 0.f};
  constexpr const glm::vec3 backward = -forward;

  constexpr const glm::vec3 up{0.f, 0.f, 1.f};
  constexpr const glm::vec3 down = -up;

  // NOTE: shadows
  // Probably want to avoid shadows on pre-rendered objects for now in order to avoid
  // contradictions with varying lighting environments during run-time.

  struct Ray
  {
    glm::vec3 origin, unitDirection;
  };

  struct Intersection
  {
    glm::vec3 position, normal;
    glm::vec3 diffuse;
    float distance;
  };

  struct Intersectable
  {
    virtual ~Intersectable() {}

    virtual std::optional<Intersection> intersect(Ray ray) const = 0;
  };

  struct DirectionalLight
  {
    DirectionalLight(glm::vec3 direction, glm::vec3 intensity)
      : ndirection{-direction}, intensity{intensity} {}

    glm::vec3 calculate(glm::vec3 position, glm::vec3 normal) const
    {
      return intensity * glm::dot(normal, ndirection);
    }

  private:
    const glm::vec3 ndirection, intensity;
  };

  struct OrthogonalCamera
  {
    // position is always at origin
    glm::vec3 normal;
    glm::vec3 xstep, ystep; // change in world coordinates from change in screen pixel coordinates

    // TODO: this way of dealing with depth may be over-complicated.
    // Instead it might be simpler to always assume that the camera plane intersects the origin,
    // and then allow intersectable to return positive or negative distance values which correspond 1:1 with the depth value.
    // Then to store in a byte a bias of +127 so that a pixel exactly at the origin has a stored depth of 127,
    // and the closest pixel that can be stored would have a depth of 0 (-127) and the farthest a depth of 254 (+127)
    // with the value 255 being a special value to indicate transparency.
    void render(
      const ViewOfCpuImageWithDepth &destImage,
      Function<std::optional<Intersection>(Ray ray)> auto &&intersect,
      const glm::vec3 minLight,
      const DirectionalLight *directionalLights,
      int numDirectionalLights)
    const
    {
      Ray ray{.unitDirection = this->normal};

      for (int y = 0; y < destImage.h; ++y)
      {
        glm::vec3 yOffset = (destImage.h * -0.5f + y + 0.5f) * ystep;

        for (int x = 0; x < destImage.w; ++x)
        {
          glm::vec3 xOffset = (destImage.w * -0.5f + x + 0.5f) * xstep;
          ray.origin = yOffset + xOffset; // this camera's origin is always the world origin

          uint32_t drgb;

          if (std::optional<Intersection> i = intersect(ray))
          {
            glm::vec3 lightSum{};

            for (int il = 0; il < numDirectionalLights; ++il)
              lightSum += directionalLights[il].calculate(i->position, i->normal);

            lightSum = glm::clamp(lightSum, minLight, glm::vec3{1.f});
            glm::vec3 color = 255.f * glm::clamp(lightSum * i->diffuse, glm::vec3{0.f}, glm::vec3{1.f});

            // old way of calculating depth from minDepth, maxDepth
            //uint8_t depth = (uint8_t)glm::clamp((i->distance - minDepth) * rDepthRange, 0.f, 255.f);
            uint8_t depth = (uint8_t)(127.f + glm::clamp(i->distance, -127.f, 128.f));

            drgb = (depth << 24) | (uint32_t(color.x) << 16) | (uint32_t(color.y) << 8) | uint32_t(color.z);
          }
          else
            drgb = 0xff000000;

          int dindex = y * destImage.w + x;
          destImage.drgb[dindex] = drgb;
        }
      }
    }
  };
}
