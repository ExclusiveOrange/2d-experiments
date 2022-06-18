#pragma once

#include "MixByte.hpp"

namespace drawing::blending
{
  struct MixArgb
  {
    static
    uint32_t mix(uint32_t argb0, uint32_t argb1, uint8_t t)
    {
      uint32_t t32 = (t << 24) | (t << 16) | (t << 8) | t;
      return MixByte::mix(argb0, argb1, t32);
    }
  };
}
