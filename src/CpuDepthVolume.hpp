#pragma once

#include <cstdint>
#include <memory>

struct ViewOfCpuDepthVolume
{
  // (depthAndThickness & 0xff) : depth in 0-255
  // (depthAndThickness >> 8) : thickness in 1-255; value 0 means transparent
  // depth is the near depth, depth + thickness is the far depth
  uint16_t *depthAndThickness;
  int w, h;
};

struct CpuDepthVolume
{
  CpuDepthVolume(int w, int h)
    : depthvolume{std::make_unique<uint16_t[]>(w * h)}
    , w{w}, h{h} {}

  [[nodiscard]]
  ViewOfCpuDepthVolume
  getUnsafeView() const {return {.depthAndThickness = depthvolume.get(), .w = w, .h = h};}

private:
  const std::unique_ptr<uint16_t[]> depthvolume;
  const int w, h;
};