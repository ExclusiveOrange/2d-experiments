#pragma once

#include <cstdint>
#include <cstring>
#include <memory>

#include "CpuImageWithDepth.hpp"

// ImageWithDepth is transparent where depth == 255.
// A drawing optimization is possible if contiguous strips where depth == 255
// can be skipped entirely.
// Memory usage is not important so don't worry about trying to compress the image,
// this optimization will make the total memory used larger.

// 2-way distance field; left->right and right->left
// top->down:
//   value (signed byte):
//     -n: next row with non-255 depth is at least n rows down
//     +n: next col with non-255 depth is at least n cols right

// TODO: figure this out
// needs to support clipping;
// maybe clipping could be made fast by a structure that can be traversed in any direction:
//   left->right->top->down
//   right->left->top->down
//   left->right->bottom->up
//   right->left->bottom->up

struct ViewOfCpuSparseImageWithDepth
{
  // size [h]: number of rows down/up to skip
  uint8_t *rowGapsDown;
  uint8_t *rowGapsUp;

  // size [w*h]: number of cols right/left to skip
  uint8_t *colGapsRight;
  uint8_t *colGapsLeft;

  uint32_t *drgb; // same as from ViewOfCpuImageWithDepth
  int w, h;
};

// note: use std::optional to contain this if you want a replaceable object, then use std::optional.emplace(...)
struct CpuSparseImageWithDepth
{
  CpuSparseImageWithDepth(const ViewOfCpuImageWithDepth &imageWithDepth)
    : w{imageWithDepth.w}, h{imageWithDepth.h}
    , drgb{std::make_unique<uint32_t[]>(w * h)}
    , gaps{std::make_unique<uint8_t[]>(h + h + w * h + w * h)}
    , view{
      .rowGapsDown = gaps.get() + 0,
      .rowGapsUp = gaps.get() + h,
      .colGapsRight = gaps.get() + h + h,
      .colGapsLeft = gaps.get() + h + h + w * h,
      .drgb = drgb.get(),
      .w = w, .h = h}
  {
    memcpy(drgb.get(), imageWithDepth.drgb, w * h * sizeof(uint32_t));

    // measure gaps from bottom-left to top-right, setting view.colGapsRight and view.rowGapsUp
    {
      int lastUnsetY = 0;

      for (int y = 0; y < h; ++y)
      {
        int lastUnsetX = 0;
        bool rowTransparent = true;

        for (int x = 0; x < w; ++x)
          if (view.drgb[y * w + x] < 0xff000000) // not transparent
          {
            rowTransparent = false;

            for (int dx = x - lastUnsetX; dx > 0; --dx)
              view.colGapsRight[y * w + x - dx] = (uint8_t)std::min(dx, 255);

            view.colGapsRight[y * w + x] = 0;
            lastUnsetX = x + 1;
          }

        for (int x = lastUnsetX; x < w; ++x)
          view.colGapsRight[y * w + x] = 255;

        if (!rowTransparent)
        {
          for (int dy = y - lastUnsetY; dy > 0; --dy)
            view.rowGapsUp[y - dy] = (uint8_t)std::min(dy, 255);

          view.rowGapsUp[y] = 0;
          lastUnsetY = y + 1;
        }
      }

      for (int y = lastUnsetY; y < h; ++y)
        view.rowGapsUp[y] = 255;
    }

    // measure gaps from top-right to bottom-left
    // TODO
  }

  ViewOfCpuSparseImageWithDepth
  getUnsafeView() const {return view;}

private:
  const int w, h;
  const std::unique_ptr<uint32_t[]> drgb;
  const std::unique_ptr<uint8_t[]> gaps; // rowGapsDown[h], rowGapsUp[h], rowGapsRight[w*h], rowGapsLeft[w*h]
  const ViewOfCpuSparseImageWithDepth view;
};