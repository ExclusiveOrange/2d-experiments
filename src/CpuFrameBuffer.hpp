#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>

#include "function_traits.hpp"

struct ViewOfCpuFrameBuffer
{
  uint32_t *image;
  int32_t *depth;
  int w, h;
  
  void clear(uint32_t argbClearValue = 0xff000000, int32_t depthClearValue = 0) const
  {
    std::fill_n(image, w * h, argbClearValue);
    std::fill_n(depth, w * h, depthClearValue);
  }
};

// note: use std::optional to contain this if you want a replaceable object, then use std::optional.emplace(...)
struct CpuFrameBuffer
{
  CpuFrameBuffer(int w, int h)
    : image{std::make_unique<uint32_t[]>(w * h)}
    , depth{std::make_unique<int32_t[]>(w * h)}
    , w{w}, h{h} {}
  
  void useWith(Function<void(const ViewOfCpuFrameBuffer &)> auto &&f)
  {
    f({.image = image.get(), .depth = depth.get(), .w = w, .h = h});
  }

private:
  const std::unique_ptr<uint32_t[]> image;
  const std::unique_ptr<int32_t[]> depth;
  const int w, h;
};
