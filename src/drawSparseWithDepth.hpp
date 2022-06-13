#pragma once

// DELETE
#include <iostream>

#include "clip.hpp"
#include "CpuFrameBuffer.hpp"
#include "CpuSparseImageWithDepth.hpp"

static void
drawSparseWithDepth(
  ViewOfCpuFrameBuffer dest, int destx, int desty,
  ViewOfCpuSparseImageWithDepth src, int16_t srcdepthbias)
{
  const int minsy = clipMin(desty, dest.h, src.h);
  const int maxsy = clipMax(desty, dest.h, src.h);
  const int minsx = clipMin(destx, dest.w, src.w);
  const int maxsx = clipMax(destx, dest.w, src.w);

  // TODO: optimize pointers to minimize index calculations since the compiler is poor at this

  // TODO: delete
  using namespace std;

  for (int y = minsy + src.rowGapsUp[minsy]; y < maxsy; y += 1 + src.rowGapsUp[y])
    for (int x = minsx + src.colGapsRight[y * src.w + minsx]; x < maxsx; x += 1 + src.colGapsRight[y * src.w + x])
      if (uint32_t sdrgb = src.drgb[y * src.w + x]; sdrgb < 0xff000000)
      {
        int idest = (y + desty) * dest.w + x + destx;

        if (int16_t sdepth = (sdrgb >> 24) + srcdepthbias, ddepth = dest.depth[idest]; sdepth < ddepth)
        {
          dest.image[idest] = 0xff000000 | (sdrgb & 0xffffff);
          dest.depth[idest] = sdepth;
        }
      }
}
