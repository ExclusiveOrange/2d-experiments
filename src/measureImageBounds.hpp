#pragma once

#include "CpuImageWithDepth.hpp"

static void
measureImageBounds(
  const ViewOfCpuImageWithDepth &image,
  int *minx, int *miny,
  int *width, int *height)
{
  int _minx = image.w - 1;
  int _maxx = 0;
  int _miny = image.h - 1;
  int _maxy = 0;

  for (int y = 0; y < image.h; ++y)
  {
    bool any = false;

    for (int x = 0; x < image.w; ++x)
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
  *miny = _miny;
  *width = _maxx - _minx + 1;
  *height = _maxy - _miny + 1;
}
