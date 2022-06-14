#pragma once

#include <stdexcept>

#include "CpuImageWithDepth.hpp"

static void
copySubImageWithDepth(
  ViewOfCpuImageWithDepth dest, int destx, int desty,
  ViewOfCpuImageWithDepth src, int srcx, int srcy, int srcw, int srch)
{
  if (destx < 0 || destx >= dest.w || desty < 0 || desty >= dest.h)
    throw std::runtime_error("destx or desty out of bounds");

  if (srcx < 0 || srcx + srcw > src.w || srcy < 0 || srcy + srch > src.h)
    throw std::runtime_error("srcx or srcy or srcw or srch out of bounds");

  if (destx + srcw > dest.w || desty + srch > dest.h)
    throw std::runtime_error("destx + srcw > dest.w or desty + srch > dest.h");

  for (int y = 0; y < srch; ++y)
    for (int x = 0; x < srcw; ++x)
    {
      int sindex = (y + srcy) * src.w + x + srcx;
      int dindex = (y + desty) * dest.w + x + destx;

      dest.drgb[dindex] = src.drgb[sindex];
    }
}

