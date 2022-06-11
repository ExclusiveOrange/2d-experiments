#pragma once

#include "CpuFrameBuffer.hpp"
#include "CpuImageWithDepth.hpp"

static void
drawWithDepth(
  ViewOfCpuFrameBuffer dest, int destx, int desty,
  ViewOfCpuImageWithDepth src, int16_t srcdepthbias)
{
  // clip
  int minsy = desty < 0 ? -desty : 0;
  int maxsy = (dest.h - desty) < src.h ? (dest.h - desty) : src.h;
  int minsx = destx < 0 ? -destx : 0;
  int maxsx = (dest.w - destx) < src.w ? (dest.w - destx) : src.w;

  // optimized for looping
  int sy = minsy;
  uint32_t *__restrict psrc = src.drgb + sy * src.w;
  uint32_t *__restrict pdestimage = dest.image + (desty + sy) * dest.w + destx;
  int16_t *pdestdepth = dest.depth + (desty + sy) * dest.w + destx;

  for (; sy < maxsy; ++sy, psrc += src.w, pdestimage += dest.w, pdestdepth += dest.w)
    for (int sx = minsx; sx < maxsx; ++sx)
      // source is transparent if source depth == 255, else test against dest
      if (uint32_t sdrgb = psrc[sx]; sdrgb < 0xff000000)
        if (int16_t sdepth = (sdrgb >> 24) + srcdepthbias, ddepth = pdestdepth[sx]; sdepth < ddepth)
        {
          // depth test passed: overwrite dest image and dest depth
          pdestimage[sx] = 0xff000000 | (sdrgb & 0xffffff);
          pdestdepth[sx] = sdepth;
        }
}
