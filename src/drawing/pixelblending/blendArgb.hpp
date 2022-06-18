#pragma once

#include <concepts>
#include <cstdint>

#if defined(__GCC__) || defined(__clang__)
#include "gcc_vec_types.hpp"

#endif

namespace drawing::pixelblending
{
  // Interpolates between bytes using a byte interpolant in 0-255.
  struct LerpByte
  {
    static
    uint8_t lerp(uint8_t a, uint8_t b, uint8_t t)
    {
      return ((a * (255 - t)) + (b * t)) / 255;
    }

    static
    uint32_t
    lerp(uint32_t a, uint32_t b, uint32_t t)
    {
      union {uint8_t v[4]; uint32_t s;} _a{.s=a}, _b{.s=b}, _t{.s=t}, r;
      for (int i = 0; i < 4; ++i)
        r.v[i] = lerp(_a.v[i], _b.v[i], _t.v[i]);
      return r.s;
    }

#if defined(__GCC__) || defined(__clang__)
    template<GccVector V>
    static
    V lerp(V a, V b, V t)
    {
      using bigger = typename gcc_vector_traits<V>::bigger_type;
      bigger a_{__builtin_convertvector(a, bigger)};
      bigger b_{__builtin_convertvector(b, bigger)};
      bigger t_{__builtin_convertvector(t, bigger)};

      return __builtin_convertvector(((a_ * (255 - t_) + (b_ * t_)) / 255), V);
    }
#endif
  };

  // TODO: delete
//#if defined(__GCC__) || defined(__clang__)
//
//  static
//  uint32_t
//  blendArgb(uint32_t argb0, uint32_t argb1, uint8_t t)
//  {
//    // GCC extension for vectors, also supported by clang (not sure min version but clang-cl with VS2022 seems okay with it).
//    // This version is much faster than the non-vector version below at least on compilers which support it.
//
//    using uint8_4 = uint8_t __attribute__((vector_size(4 * sizeof(uint8_t))));
//    using uint16_4 = uint16_t __attribute__((vector_size(4 * sizeof(uint16_t))));
//
//    union {uint32_t u32; uint8_4 u8_4;}
//      a8{.u32 = argb0}, b8{.u32 = argb1}, r;
//
//    uint16_4 a16{__builtin_convertvector(a8.u8_4, uint16_4)};
//    uint16_4 b16{__builtin_convertvector(b8.u8_4, uint16_4)};
//    uint16_4 f16{(a16 * uint16_t(255 - t) + b16 * uint16_t(t)) / uint16_t(255)};
//    r.u8_4 = __builtin_convertvector(f16, uint8_4);
//
//    return r.u32;
//  };
//
//#else // not defined(__GCC__) and not defined(__clang__)
//
//  static
//  uint32_t
//  blendArgb(uint32_t argb0, uint32_t argb1, uint8_t t)
//  {
//    // This function is much slower with MSVC for some reason.
//    // I tried several variations and they were all bad compared to clang-cl.
//
//    union Prism {struct {uint8_t a, r, g, b;}; uint32_t argb;}
//      p0{.argb = argb0}, p1{.argb = argb1};
//
//    return Prism{
//      .a = uint8_t(((uint16_t)p0.a * uint16_t(255 - t) + (uint16_t)p1.a * uint16_t(t)) / 255),
//      .r = uint8_t(((uint16_t)p0.r * uint16_t(255 - t) + (uint16_t)p1.r * uint16_t(t)) / 255),
//      .g = uint8_t(((uint16_t)p0.g * uint16_t(255 - t) + (uint16_t)p1.g * uint16_t(t)) / 255),
//      .b = uint8_t(((uint16_t)p0.b * uint16_t(255 - t) + (uint16_t)p1.b * uint16_t(t)) / 255)}.argb;
//  }
//
//#endif
} // namespace drawing