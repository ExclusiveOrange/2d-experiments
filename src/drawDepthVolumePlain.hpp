#pragma once

#include <glm/vec3.hpp>

#include "clip.hpp"
#include "CpuFrameBuffer.hpp"
#include "CpuDepthVolume.hpp"

static void
drawDepthVolumePlain(
  ViewOfCpuFrameBuffer dest, int destx, int desty,
  ViewOfCpuDepthVolume src, int16_t srcdepthbias,
  glm::vec3 rgb)
{
  int minsy = clipMin(desty, dest.h, src.h);
  int maxsy = clipMax(desty, dest.h, src.h);
  int minsx = clipMin(destx, dest.w, src.w);
  int maxsx = clipMax(destx, dest.w, src.w);

  // TODO
}
