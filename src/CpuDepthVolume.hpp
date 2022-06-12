#pragma once

#include <cstdint>
#include <memory>

struct DepthVolumePixel {uint8_t near, thickness;}; // set thickness == 0 for transparency

struct ViewOfCpuDepthVolume
{
  DepthVolumePixel *depthvolume;
  int w, h;
};

struct CpuDepthVolume
{
  CpuDepthVolume(int w, int h)
    : depthvolume{std::make_unique<DepthVolumePixel[]>(w * h)}
    , w{w}, h{h} {}

  ViewOfCpuDepthVolume
  getUnsafeView() const {return {.depthvolume = depthvolume.get(), .w = w, .h = h};}

private:
  const std::unique_ptr<DepthVolumePixel[]> depthvolume;
  const int w, h;
};