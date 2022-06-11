#pragma once

#include <cstdint>
#include <memory>

struct ViewOfCpuImageWithDepth
{
  uint32_t *drgb; // depth instead of alpha: 0-254 == test, 255 == cull (transparent)
  int w, h;
};

// note: use std::optional to contain this if you want a replaceable object, then use std::optional.emplace(...)
struct CpuImageWithDepth
{
  CpuImageWithDepth(int w, int h)
    : drgb{std::make_unique<uint32_t[]>(w * h)}
    , w{w}, h{h} {}
  
  ViewOfCpuImageWithDepth
  getUnsafeView() const {return {.drgb = drgb.get(), .w = w, .h = h};}

private:
  const std::unique_ptr<uint32_t[]> drgb;
  const int w, h;
};
