#pragma once

#include <glm/common.hpp>
#include <glm/vec3.hpp>

#include <function_traits.hpp>

#include "clip.hpp"
#include "../CpuFrameBuffer.hpp"
#include "../CpuDepthVolume.hpp"

namespace drawing
{
  static void
  drawDepthVolume(
    ViewOfCpuFrameBuffer dest, int destx, int desty,
    ViewOfCpuDepthVolume src, int16_t srcdepthbias,
    Function<uint32_t(uint32_t destArgb, uint8_t thickness)> auto &&argbFromThickness)
  {
    const int minsy = clipMin(desty, dest.h, src.h);
    const int maxsy = clipMax(desty, dest.h, src.h);
    const int minsx = clipMin(destx, dest.w, src.w);
    const int maxsx = clipMax(destx, dest.w, src.w);

    for (int y = minsy; y < maxsy; ++y)
      for (int x = minsx; x < maxsx; ++x)
      {
        uint16_t srcDepthAndThickness = src.depthAndThickness[y * src.w + x];
        uint8_t thickness = srcDepthAndThickness >> 8;

        if (thickness == 0)
          continue;

        int dindex = (desty + y) * dest.w + (destx + x);
        int destDepth = dest.depth[dindex];

        int srcDepthBiased = (srcDepthAndThickness & 0xff) + srcdepthbias;

        if (int destMinusSrcDepth = destDepth - srcDepthBiased; destMinusSrcDepth > 0)
        {
          uint32_t destArgb = dest.image[dindex];
          uint8_t clippedThickness = glm::min(destMinusSrcDepth, (int)thickness);
          dest.image[dindex] = argbFromThickness(destArgb, clippedThickness);
          dest.depth[dindex] = srcDepthBiased;
        }
      }
  }
}
