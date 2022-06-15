#pragma once

#include "clip.hpp"
#include "CpuFrameBuffer.hpp"
#include "CpuImageWithDepth.hpp"

// NOTE: MSVC (at least) is incredibly stubborn about auto-vectorizing loops.
// Using Godbolt.org I am unable to compel MSVC to auto-vectorize even a trivial loop with __restrict pointers, it simply refuses with a bullshit 1200 code.
// NOTE: xsimd looks nice from a distance but it has serious limitations including not support a large number of SIMD instructions / features,
// making it difficult or impossible to do something like convert uint32_t to int16_t and then do something useful with the result.
// For example there is no shuffle or permute, only xsimd::swizzle, and it doesn't compile for an int16_t batch due to some uknown limitation.

static void
drawWithDepth(
  ViewOfCpuFrameBuffer dest, int destx, int desty,
  ViewOfCpuImageWithDepth src, int16_t srcdepthbias)
{
  const int minsy = clipMin(desty, dest.h, src.h);
  const int maxsy = clipMax(desty, dest.h, src.h);
  const int minsx = clipMin(destx, dest.w, src.w);
  const int maxsx = clipMax(destx, dest.w, src.w);

  if (minsx >= maxsx || minsy >= maxsy)
    return;


  // simd testing
  {
    uint32_t src[8];
    for (int i = 0; i < 8; ++i)
      src[i] = (100 + i) << 24;

    int16_t bias = 500;

    // TODO: figure out SIMD, try to break it out into very simple stand-alone functions which can be customized for each platform

    return;
  }

  // optimized pointers (might not be needed with simd)
  uint32_t *__restrict psrc = src.drgb + minsy * src.w + minsx;
  uint32_t *__restrict pdestimage = dest.image + (desty + minsy) * dest.w + destx + minsx;
  int16_t *pdestdepth = dest.depth + (desty + minsy) * dest.w + destx + minsx;

  for (int sy = minsy; sy < maxsy; ++sy, psrc += src.w, pdestimage += dest.w, pdestdepth += dest.w)
  {
    //for (size_t i = 0; i < vecWidth; i += simdSize)
    //{
      //auto sdrgb = xs::load_unaligned(&psrc[i]);
      //auto ddepth = xs::load_as<uint32_t>(&pdestdepth[i], xs::unaligned_mode());
      //auto sdepthraw = xs::batch<int16_t>(sdrgb >> 24) + srcdepthbias;

      //std::cout << "sdepthraw: " << sdepthraw << std::endl;
      // TODO
    //}

    //for (size_t i = vecWidth; i < width; ++i)
    //  if (uint32_t sdrgb = psrc[i]; sdrgb < 0xff000000)
    //    if (int16_t sdepth = int16_t((sdrgb >> 24) + srcdepthbias), ddepth = pdestdepth[i]; sdepth < ddepth)
    //    {
    //      pdestimage[i] = 0xff000000 | sdrgb;
    //      pdestdepth[i] = sdepth;
    //    }
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
