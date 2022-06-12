#pragma once

#include "clip.hpp"
#include "CpuFrameBuffer.hpp"
#include "CpuImageWithDepth.hpp"

static void
drawWithoutDepth(
  ViewOfCpuFrameBuffer dest, int destx, int desty,
  ViewOfCpuImageWithDepth src)
{
  const int minsy = clipMin(desty, dest.h, src.h);
  const int maxsy = clipMax(desty, dest.h, src.h);
  const int minsx = clipMin(destx, dest.w, src.w);
  const int maxsx = clipMax(destx, dest.w, src.w);

  // for each non-clipped pixel in source...
  for (int sy = minsy; sy < maxsy; ++sy)
  {
    int drowstart = (desty + sy) * dest.w + destx;
    int srowstart = sy * src.w;
    
    for (int sx = minsx; sx < maxsx; ++sx)
    {
      uint32_t sdrgb = src.drgb[srowstart + sx];
      
      // source is transparent if source depth == 255, else test against dest
      if (sdrgb < 0xff000000)
      {
        uint32_t sargb = 0xff000000 | (sdrgb & 0xffffff);
        dest.image[drowstart + sx] = sargb;
      }
    }
  }
}
