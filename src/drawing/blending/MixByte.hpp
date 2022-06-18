#pragma once

#include <concepts>
#include <cstdint>

#if defined(__GCC__) || defined(__clang__)
#include "gcc_vec_types.hpp"

#endif

namespace drawing::blending
{
  // Interpolates between bytes using a byte interpolant in 0-255.
  struct MixByte
  {
    static
    uint8_t mix(uint8_t a, uint8_t b, uint8_t t)
    {
      return ((a * (255 - t)) + (b * t)) / 255;
    }

    static
    uint32_t
    mix(uint32_t a, uint32_t b, uint32_t t)
    {
      union {uint8_t v[4]; uint32_t s;} _a{.s=a}, _b{.s=b}, _t{.s=t}, r;
      for (int i = 0; i < 4; ++i)
        r.v[i] = mix(_a.v[i], _b.v[i], _t.v[i]);
      return r.s;
    }

#if defined(__GCC__) || defined(__clang__)
    template<GccVector V>
    static
    V mix(V a, V b, V t)
    {
      using bigger = typename gcc_vector_traits<V>::bigger_type;
      bigger a_{__builtin_convertvector(a, bigger)};
      bigger b_{__builtin_convertvector(b, bigger)};
      bigger t_{__builtin_convertvector(t, bigger)};

      return __builtin_convertvector(((a_ * (255 - t_) + (b_ * t_)) / 255), V);
    }
#endif
  };
} // namespace drawing::pixelblending