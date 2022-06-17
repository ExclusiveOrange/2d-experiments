#pragma once

#ifdef __AVX2__
#include <immintrin.h>
#endif

#include "clip.hpp"
#include "../CpuFrameBuffer.hpp"
#include "../CpuImageWithDepth.hpp"

namespace drawing
{
#ifdef __AVX2__
  static void
  drawWithDepth(
    ViewOfCpuFrameBuffer dest, int destx, int desty,
    ViewOfCpuImageWithDepth src, int16_t srcdepthbias)
  {
    // 2022.06.15 Atlee: This handwritten SIMD version is well more than twice as fast as the manually optimized non-SIMD version.
    // There is probably more room for improvement.

    const int minsy = clipMin(desty, dest.h, src.h);
    const int maxsy = clipMax(desty, dest.h, src.h);
    const int minsx = clipMin(destx, dest.w, src.w);
    const int maxsx = clipMax(destx, dest.w, src.w);

    if (minsx >= maxsx || minsy >= maxsy)
      return;

    const int width = maxsx - minsx;

    // SIMD specials
    constexpr size_t simdSize = 8;
    const size_t vecWidth = width - width % simdSize;
    const __m256i u32_0x000000ff = _mm256_set1_epi32(0x000000ff); // to compare 8 bit depth from source with 255 after >> 24
    const __m256i u32_0xff000000 = _mm256_set1_epi32(0xff << 24); // sets alpha channel to 255 when storing src to dest image
    const __m128i src_depth_bias = _mm_set1_epi16(srcdepthbias);

    // pre-calculate fixed pointer offsets
    uint32_t *__restrict psrc = src.drgb + minsy * src.w + minsx;
    uint32_t *__restrict pdestimage = dest.image + (desty + minsy) * dest.w + destx + minsx;
    int16_t *pdestdepth = dest.depth + (desty + minsy) * dest.w + destx + minsx;

    for (int sy = minsy; sy < maxsy; ++sy, psrc += src.w, pdestimage += dest.w, pdestdepth += dest.w)
    {
      for (size_t i = 0; i < vecWidth; i += simdSize)
      {
        __m256i src_drgb = _mm256_loadu_si256((__m256i *)(psrc + i));

        __m256i src_depth_unbiased_32 = _mm256_srli_epi32(src_drgb, 24); // src_drgb >> 24
        __m256i src_depth_255_mask_32 = _mm256_cmpeq_epi32(src_depth_unbiased_32, u32_0x000000ff);

        if (_mm256_testc_si256(src_depth_255_mask_32, _mm256_set1_epi64x(-1)))
          continue; // all src_depth == 255 (transparent)

        __m128i dst_depth = _mm_loadu_si128((__m128i *)(pdestdepth + i));

        __m128i src_depth_255_mask_32_1 = _mm256_extracti128_si256(src_depth_255_mask_32, 1);
        __m128i src_depth_255_mask_32_0 = _mm256_castsi256_si128(src_depth_255_mask_32);
        __m128i src_depth_255_mask_16 = _mm_packs_epi16(src_depth_255_mask_32_0, src_depth_255_mask_32_1);

        __m128i src_depth_unbiased_32_1 = _mm256_extracti128_si256(src_depth_unbiased_32, 1);
        __m128i src_depth_unbiased_32_0 = _mm256_castsi256_si128(src_depth_unbiased_32);
        __m128i src_depth_unbiased_16 = _mm_packs_epi32(src_depth_unbiased_32_0, src_depth_unbiased_32_1);
        __m128i src_depth_biased = _mm_adds_epi16(src_depth_unbiased_16, src_depth_bias);
        __m128i src_depth_lt_dst_depth_mask = _mm_cmpgt_epi16(dst_depth, src_depth_biased);

        if (_mm_testz_si128(src_depth_lt_dst_depth_mask, _mm_set1_epi64x(-1)))
          continue; // all src_depth >= dst_depth

        __m256i dst_argb = _mm256_loadu_si256((__m256i *)(pdestimage + i));

        __m128i src_final_mask_16 = _mm_andnot_si128(src_depth_255_mask_16, src_depth_lt_dst_depth_mask);

        __m128i src_depth_or_dst_depth = _mm_blendv_epi8(dst_depth, src_depth_biased, src_final_mask_16);
        __m256i src_argb = _mm256_or_si256(src_drgb, u32_0xff000000);
        __m256i src_final_mask_32 = _mm256_cvtepi16_epi32(src_final_mask_16);
        __m256i src_argb_or_dst_argb = _mm256_blendv_epi8(dst_argb, src_argb, src_final_mask_32);

        _mm_storeu_si128((__m128i *)(pdestdepth + i), src_depth_or_dst_depth);
        _mm256_storeu_si256((__m256i *)(pdestimage + i), src_argb_or_dst_argb);
      }

      for (size_t i = vecWidth; i < width; ++i)
        if (uint32_t sdrgb = psrc[i]; sdrgb < 0xff000000)
          if (int16_t sdepth = int16_t((sdrgb >> 24) + srcdepthbias), ddepth = pdestdepth[i]; sdepth < ddepth)
          {
            pdestimage[i] = 0xff000000 | sdrgb;
            pdestdepth[i] = sdepth;
          }
    }
  }
#else // else not __AVX2__
  // optimized but not for SIMD
  static void
  drawWithDepth(
    ViewOfCpuFrameBuffer dest, int destx, int desty,
    ViewOfCpuImageWithDepth src, int16_t srcdepthbias)
  {
    // 2022.06.15 Atlee: This manually optimized version is 30-50% faster than the naive version with either MSVC or Clang.

    const int minsy = clipMin(desty, dest.h, src.h);
    const int maxsy = clipMax(desty, dest.h, src.h);
    const int minsx = clipMin(destx, dest.w, src.w);
    const int maxsx = clipMax(destx, dest.w, src.w);

    // optimized for looping
    int sy = minsy;
    uint32_t *__restrict psrc = src.drgb + sy * src.w;
    uint32_t *__restrict pdestimage = dest.image + (desty + sy) * dest.w + destx;
    int16_t *pdestdepth = dest.depth + (desty + sy) * dest.w + destx;

    for (; sy < maxsy; ++sy, psrc += src.w, pdestimage += dest.w, pdestdepth += dest.w)
      for (int sx = minsx; sx < maxsx; ++sx)
        // source is transparent if source depth == 255, else test against dest
        if (uint32_t sdrgb = psrc[sx]; sdrgb < 0xff000000)
          if (int16_t sdepth = (sdrgb >> 24) + srcdepthbias, ddepth = pdestdepth[sx]; sdepth < ddepth)
          {
            // depth test passed: overwrite dest image and dest depth
            pdestimage[sx] = 0xff000000 | sdrgb;
            pdestdepth[sx] = sdepth;
          }
  }
#endif
}