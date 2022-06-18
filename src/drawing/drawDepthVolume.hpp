#pragma once

#include <glm/common.hpp>
#include <glm/vec3.hpp>

#include <function_traits.hpp>

#include "clip.hpp"
#include "../CpuFrameBuffer.hpp"
#include "../CpuDepthVolume.hpp"

namespace drawing
{
  //static void
  //drawDepthVolume(
  //  ViewOfCpuFrameBuffer dest, int destx, int desty,
  //  ViewOfCpuDepthVolume src, int16_t srcdepthbias,
  //  Function<uint32_t(uint32_t destArgb, uint8_t thickness)> auto &&argbFromThickness)
  //{
  //  const int minsy = clipMin(desty, dest.h, src.h);
  //  const int maxsy = clipMax(desty, dest.h, src.h);
  //  const int minsx = clipMin(destx, dest.w, src.w);
  //  const int maxsx = clipMax(destx, dest.w, src.w);
  //
  //  constexpr size_t blockSize = 8;
  //  const size_t width = maxsx - minsx;
  //  const size_t blockWidth = width - width % blockSize;
  //
  //  for (int y = minsy; y < maxsy; ++y)
  //  {
  //    uint16_t *__restrict psrc = src.depthAndThickness + y * src.w + minsx;
  //    int16_t *__restrict pdestdepth = dest.depth + (desty + y) * dest.w + destx + minsx;
  //    uint32_t *__restrict pdestargb = dest.image + (desty + y) * dest.w + destx + minsx;
  //
  //    for (size_t i = 0; i < blockWidth; i += blockSize)
  //    {
  //      // TODO: block of blockSize values
  //    }
  //
  //    for (size_t i = blockWidth; i < width; ++i)
  //    {
  //      // TODO: single value
  //      struct
  //
  //
  //    }
  //
  //    for (int x = minsx; x < maxsx; ++x)
  //    {
  //      //uint16_t srcDepthAndThickness = src.depthAndThickness[y * src.w + x];
  //      uint16_t srcDepthAndThickness = psrc[x];
  //      uint8_t thickness = srcDepthAndThickness >> 8;
  //
  //      if (thickness == 0)
  //        continue;
  //
  //      //int dindex = (desty + y) * dest.w + (destx + x);
  //      //int dindex = dindexrow + x;
  //      //int destDepth = dest.depth[dindex];
  //      int destDepth = pdestdepth[x];
  //
  //      int srcDepthBiased = (srcDepthAndThickness & 0xff) + srcdepthbias;
  //
  //      if (int destMinusSrcDepth = destDepth - srcDepthBiased; destMinusSrcDepth > 0)
  //      {
  //        //uint32_t destArgb = dest.image[dindex];
  //        uint32_t destArgb = pdestargb[x];
  //        uint8_t clippedThickness = glm::min(destMinusSrcDepth, (int)thickness);
  //        pdestargb[x] = argbFromThickness(destArgb, clippedThickness);
  //        pdestdepth[x] = srcDepthBiased;
  //        //dest.image[dindex] = argbFromThickness(destArgb, clippedThickness);
  //        //dest.depth[dindex] = srcDepthBiased;
  //      }
  //    }
  //  }
  //}

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

    constexpr size_t blockSize = 8;
    const size_t width = maxsx - minsx;
    const size_t blockWidth = width - width % blockSize;

    for (int y = minsy; y < maxsy; ++y)
    {
      uint16_t *__restrict psrc = src.depthAndThickness + y * src.w;
      int16_t *__restrict pdestdepth = dest.depth + (desty + y) * dest.w + destx;
      uint32_t *__restrict pdestargb = dest.image + (desty + y) * dest.w + destx;
      //int dindexrow = (desty + y) * dest.w + destx;


      for (int x = minsx; x < maxsx; ++x)
      {
        //uint16_t srcDepthAndThickness = src.depthAndThickness[y * src.w + x];
        uint16_t srcDepthAndThickness = psrc[x];
        uint8_t thickness = srcDepthAndThickness >> 8;

        if (thickness == 0)
          continue;

        //int dindex = (desty + y) * dest.w + (destx + x);
        //int dindex = dindexrow + x;
        //int destDepth = dest.depth[dindex];
        int destDepth = pdestdepth[x];

        int srcDepthBiased = (srcDepthAndThickness & 0xff) + srcdepthbias;

        if (int destMinusSrcDepth = destDepth - srcDepthBiased; destMinusSrcDepth > 0)
        {
          //uint32_t destArgb = dest.image[dindex];
          uint32_t destArgb = pdestargb[x];
          uint8_t clippedThickness = glm::min(destMinusSrcDepth, (int)thickness);
          pdestargb[x] = argbFromThickness(destArgb, clippedThickness);
          pdestdepth[x] = srcDepthBiased;
          //dest.image[dindex] = argbFromThickness(destArgb, clippedThickness);
          //dest.depth[dindex] = srcDepthBiased;
        }
      }
    }
  }
}
