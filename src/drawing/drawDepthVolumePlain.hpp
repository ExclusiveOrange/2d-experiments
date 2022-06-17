#pragma once

#include <glm/vec3.hpp>

#include "clip.hpp"
#include "../CpuFrameBuffer.hpp"
#include "../CpuDepthVolume.hpp"

namespace drawing
{
  static void
  drawDepthVolumePlain(
    ViewOfCpuFrameBuffer dest, int destx, int desty,
    ViewOfCpuDepthVolume src, int16_t srcdepthbias,
    glm::vec3 rgb)
  {
    const int minsy = clipMin(desty, dest.h, src.h);
    const int maxsy = clipMax(desty, dest.h, src.h);
    const int minsx = clipMin(destx, dest.w, src.w);
    const int maxsx = clipMax(destx, dest.w, src.w);

    // TODO
  }
}
