#pragma once

#include "CpuImageWithDepth.hpp"

static void
measureImageBounds(
  const ViewOfCpuImageWithDepth &image,
  int *minx, int *maxx,
  int *miny, int *maxy)
{
  int _minx = image.w - 1;
  int _maxx = 0;
  int _miny = image.h - 1;
  int _maxy = 0;

  for (int y = 0; y < image.h; ++y)
    for (int x = 0; x < image.w; ++x)
    {
      bool any = false;

      if (image.drgb[y * image.w + x] < 0xff000000)
      {
        any = true;
        _minx = std::min(x, _minx);
        _maxx = std::max(x, _maxx);
      }

      if (any)
      {
        _miny = std::min(y, _miny);
        _maxy = std::max(y, _maxy);
      }
    }

  *minx = _minx;
  *maxx = _maxx;
  *miny = _miny;
  *maxy = _maxy;
}
