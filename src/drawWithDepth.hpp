#pragma once

#include "CpuFrameBuffer.hpp"
#include "CpuImageWithDepth.hpp"

static void
drawWithDepth(
  ViewOfCpuFrameBuffer dest, int destx, int desty,
  ViewOfCpuImageWithDepth src, int srcdepthbias)
{
  // clip vertical
  int minsy = desty < 0 ? -desty : 0;
  int maxsy = (dest.h - desty) < src.h ? (dest.h - desty) : src.h;
  
  // clip horizontal
  int minsx = destx < 0 ? -destx : 0;
  int maxsx = (dest.w - destx) < src.w ? (dest.w - destx) : src.w;
  
  // for each non-clipped pixel in source...
  for (int sy = minsy; sy < maxsy; ++sy)
  {
    int drowstart = (desty + sy) * dest.w + destx;
    int srowstart = sy * src.w;
    
    for (int sx = minsx; sx < maxsx; ++sx)
    {
      int sindex = srowstart + sx;
      uint32_t sdrgb = src.drgb[sindex];
      
      // source is transparent if source depth == 255, else test against dest
      if (sdrgb < 0xff000000)
      {
        int dindex = drowstart + sx;
        int ddepth = dest.depth[dindex];
        
        int sdepth = ((sdrgb & 0xff000000) >> 24) + srcdepthbias;
        
        if (sdepth < ddepth)
        {
          // depth test passed: overwrite dest image and dest depth
          uint32_t sargb = 0xff000000 | (sdrgb & 0xffffff);
          dest.image[dindex] = sargb;
          dest.depth[dindex] = sdepth;
        }
      }
    }
  }
}
