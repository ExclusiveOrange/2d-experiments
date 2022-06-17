#pragma once

namespace raycasting
{
  struct DepthIntersection
  {
    float distance0, distance1; // distance along ray from ray.origin; distance0 should be <= distance1
  };
}
