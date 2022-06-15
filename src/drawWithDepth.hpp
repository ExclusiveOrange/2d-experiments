#pragma once

#include "clip.hpp"
#include "CpuFrameBuffer.hpp"
#include "CpuImageWithDepth.hpp"

static void
drawWithDepth(
  ViewOfCpuFrameBuffer dest, int destx, int desty,
  ViewOfCpuImageWithDepth src, int16_t srcdepthbias)
{
  const int minsy = clipMin(desty, dest.h, src.h);
  const int maxsy = clipMax(desty, dest.h, src.h);
  const int minsx = clipMin(destx, dest.w, src.w);
  const int maxsx = clipMax(destx, dest.w, src.w);

  const int width = maxsx - minsx;

  // optimized for looping
  int sy = minsy;
  uint32_t *__restrict psrc = src.drgb + sy * src.w;
  uint32_t *__restrict pdestimage = dest.image + (desty + sy) * dest.w + destx;
  int16_t *pdestdepth = dest.depth + (desty + sy) * dest.w + destx;

  for (; sy < maxsy; ++sy, psrc += src.w, pdestimage += dest.w, pdestdepth += dest.w)
  {
    int sx = minsx;
    int w = width;

    for (; w >= 4; sx += 4, w -= 4)
    {
      uint32_t sdrgbs[4];
      int16_t ddepths[4];
      int16_t sdepths[4];

      for (int i = 0; i < 4; ++i)
        sdrgbs[i] = psrc[sx + i];

      for (int i = 0; i < 4; ++i)
        ddepths[i] = pdestdepth[sx + i];

      for (int i = 0; i < 4; ++i)
        sdepths[i] = int16_t((sdrgbs[i] >> 24) + srcdepthbias);

      for (int i = 0; i < 4; ++i)
        if (sdepths[i] < ddepths[i])
        {
          pdestimage[sx + i] = 0xff000000 | sdrgbs[i];
          pdestdepth[sx + i] = sdepths[i];
        }
    }

    for (; w > 0; ++sx, --w)
      if (uint32_t sdrgb = psrc[sx]; sdrgb < 0xff000000)
        if (int16_t sdepth = int16_t((sdrgb >> 24) + srcdepthbias), ddepth = pdestdepth[sx]; sdepth < ddepth)
        {
          pdestimage[sx] = 0xff000000 | sdrgb;
          pdestdepth[sx] = sdepth;
        }
  }
}

// optimized but not for SIMD
//static void
//drawWithDepth(
//  ViewOfCpuFrameBuffer dest, int destx, int desty,
//  ViewOfCpuImageWithDepth src, int16_t srcdepthbias)
//{
//  const int minsy = clipMin(desty, dest.h, src.h);
//  const int maxsy = clipMax(desty, dest.h, src.h);
//  const int minsx = clipMin(destx, dest.w, src.w);
//  const int maxsx = clipMax(destx, dest.w, src.w);
//
//  // optimized for looping
//  int sy = minsy;
//  uint32_t *__restrict psrc = src.drgb + sy * src.w;
//  uint32_t *__restrict pdestimage = dest.image + (desty + sy) * dest.w + destx;
//  int16_t *pdestdepth = dest.depth + (desty + sy) * dest.w + destx;
//
//  for (; sy < maxsy; ++sy, psrc += src.w, pdestimage += dest.w, pdestdepth += dest.w)
//    for (int sx = minsx; sx < maxsx; ++sx)
//      // source is transparent if source depth == 255, else test against dest
//      if (uint32_t sdrgb = psrc[sx]; sdrgb < 0xff000000)
//        if (int16_t sdepth = (sdrgb >> 24) + srcdepthbias, ddepth = pdestdepth[sx]; sdepth < ddepth)
//        {
//          // depth test passed: overwrite dest image and dest depth
//          pdestimage[sx] = 0xff000000 | sdrgb;
//          pdestdepth[sx] = sdepth;
//        }
//}
