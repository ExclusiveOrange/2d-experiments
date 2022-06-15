//
// Created by Atlee on 6/14/2022.
//

#ifdef __AVX2__
#pragma message("AVX2 detected; assuming SSE4.2")
#include <immintrin.h>

#endif

#include <bitset>
#include <iostream>

// PERFORMANCE NOTES:
//   reference: https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html
// I am developing this on a 9700k, which is a Coffee Lake architecture, which is supposedly the same as Skylake as far as latency and CPI are concerned.
// According to the reference, the Skylake typically has much higher latency and lower throughput than previous architectures like Haswell,
// particularly with loads and stores.
// Obviously the Skylake is in practice much faster per clock so I'm not sure what's going on with that.
// Nonetheless I should mind the latencies and try to get other instructions working while waiting for loads / stores.

using namespace std;

static
void
print(const char *name, auto &&a)
{
  cout << name << "{";
  bool first = true;
  for (auto x: a)
  {
    if (first)
      first = !first;
    else
      cout << ", ";
    cout << x;
  }
  cout << "}" << endl;
}

static
void
print_i16(const char *name, __m128i x)
{
  int16_t a[8];
  _mm_storeu_si128((__m128i *)a, x);
  print(name, a);
}

static
void
print_u16(const char *name, __m128i x)
{
  uint16_t a[8];
  _mm_storeu_si128((__m128i *)a, x);
  print(name, a);
}

static
void
print_u32(const char *name, __m256i x)
{
  uint32_t a[8];
  _mm256_storeu_si256((__m256i *)a, x);
  print(name, a);
}

void simdtest1()
{
  // TEST 1 GOAL:
  // load 8 x uint32 values
  // convert to 8 x int16 values
  // store 8 x int16 values
  // status:
  //   _mm_packs_epi32 is the magic instruction to convert signed 32 bit to signed 16 bit;
  //   should be very fast

  // NOTE:
  // just use 128 _mm_packs_epi32(128, 128)

  uint32_t u32vs[8];
  int16_t i16check_8[8];

  for (int i = 0; i < 8; ++i)
    u32vs[i] = i;

  __m128i u32a = _mm_loadu_si128((__m128i *)&u32vs[0]); // L 6 CPI 0.5
  __m128i u32b = _mm_loadu_si128((__m128i *)&u32vs[4]); // L 6 CPI 0.5
  __m128i s16 = _mm_packs_epi32(u32a, u32b); // L 1 CPI 1
  _mm_storeu_si128((__m128i *)i16check_8, s16);
  print_i16("s16", s16);
  print("i16check_8", i16check_8);
}

void simdtest2()
{
  // TEST 2 GOAL:
  //   compare 2 x int16[8] arrays element-wise
  //   specifically want to know when a < b

  int16_t i16_a[8];
  int16_t i16_b[8];
  int16_t i16_c[8];

  for (int i = 0; i < 8; ++i)
  {
    i16_a[i] = (int16_t)i;
    //i16_b[i] = (int16_t)(7 - i);
    i16_b[i] = (int16_t)(i - 1);
    i16_c[i] = 0;
  }

  i16_b[3] = 4;

  __m128i a = _mm_loadu_si128((__m128i *)i16_a);
  __m128i b = _mm_loadu_si128((__m128i *)i16_b);

  print_i16("a", a);
  print_i16("b", b);

  // altb[i] = a[i] < b[i] ? 0xFFFF : 0
  __m128i altb = _mm_cmpgt_epi16(b, a);
  print_u16("a < b", altb);

  if (_mm_testz_si128(altb, _mm_set1_epi64x(-1)))
  {
    cout << "all a >= b" << endl;
    return;
  }

  // NOTE: sounds like _mm_maskmoveu_si128 (maskmovdqu) has bad performance in modern times,
  // so the common fast solution seems to be to load the dest, do a blend, and store the result back in dest

  __m128i c = _mm_loadu_si128((__m128i *)i16_c);

  // (int16)a_or_c[i] = a < b ? a[i] : c[i]
  __m128i a_or_c = _mm_blendv_epi8(c, a, altb);

  print_i16("a_or_c", a_or_c);
}

void simdtest3()
{
  // TEST 3 GOAL:
  //   compare 2 x int16[8] arrays element-wise
  //   conditionally set a uint32[8] array corresponding to previous element-wise comparison
  // status:
  //   success: _mm256_cvtepi16_epi32 converts a 16 bit mask to a 32 bit mask through sign extension

  int16_t i16_a[8];
  int16_t i16_b[8];

  uint32_t u32_c[8];
  uint32_t u32_d[8]; // u32_d[i] = i16_a[i] < i16_b[i] ? u32_c[i] : u32_d[i];

  for (int i = 0; i < 8; ++i)
  {
    i16_a[i] = (int16_t)i;
    i16_b[i] = (int16_t)(!(i & 1) ? i - 1 : i + 1);

    u32_c[i] = i + 10;
    u32_d[i] = 0;
  }

  __m128i a = _mm_loadu_si128((__m128i *)i16_a); // L 6 CPI 0.5
  __m128i b = _mm_loadu_si128((__m128i *)i16_b); // L 6 CPI 0.5

  print_i16("a", a);
  print_i16("b", b);

  __m128i altb = _mm_cmpgt_epi16(b, a); // L 1 CPI 0.5

  if (_mm_testz_si128(altb, _mm_set1_epi64x(-1))) // testz: L 3 CPU 1; set1_epi64x: is a sequence but Godbolt suggests it is very fast and maybe free
  {
    cout << "all a >= b" << endl;
    return;
  }

  __m256i altb_32 = _mm256_cvtepi16_epi32(altb); // L 3 CPI 1

  print_u32("altb_32", altb_32);

  __m256i c = _mm256_loadu_si256((__m256i*)u32_c); // L 7 CPI 0.5
  __m256i d = _mm256_loadu_si256((__m256i*)u32_d); // L 7 CPI 0.5

  __m256i c_or_d = _mm256_blendv_epi8(d, c, altb_32); // L 2 CPI 0.66
  _mm256_storeu_si256((__m256i*)u32_d, c_or_d); // L 5 CPI 1

  print("c", u32_c);
  print("d", u32_d);
}

void simdtest4(int16_t srcBias)
{
  // TEST 4 GOAL:
  //   Put together everything above and do the complete process:
  //     reading source drgb,
  //     extracting depth,
  //     if drgb.d = 255 then set mask_a,
  //     if all(mask_a) then quit
  //
  //     src_depth = drgb.d + bias,
  //     reading dest_depth,
  //     if src_depth < dest_depth then set mask_b,
  //     mask_c = mask_a XOR mask_b
  //     storing zero or more argb and depth values based on per pixel depth check.
  //
  //   uint32_t drgb = something;
  //   uint32_t d8 = drgb >> 24;
  //   int16_t d16 = d8 + bias
  //   uint32_t argb = 0xff000000 | drgb;

  uint32_t _src_drgb[8];
  uint32_t _dst_argb[8];
  int16_t _dst_depth[8];

  for (int i = 0; i < 8; ++i)
  {
    uint8_t src_depth = (i & 3) + 254;
    _src_drgb[i] = ((uint32_t)src_depth << 24) | 0x556677;
    _dst_argb[i] = 0x00112233;
    _dst_depth[i] = (int16_t)(i - 4);
  }

  // read src_drgb
  __m256i drgb = _mm256_loadu_si256((__m256i*)_src_drgb); // L 7 CPI 0.5

  // NOTE: can't figure out how to test src_drgb depth against 255 directly in SIMD since all the 32 bit integer instructions seem to be for signed integers,
  // so instead I'm doing a bit more work and then doing the test;
  // I don't think this is a big deal because it's only a few simple instructions that have to be done anyway in the case at least one pixel in source doesn't have depth 255

  // src_drgb >> 24
  __m256i src_depth_unbiased_32 = _mm256_srli_epi32(drgb, 24); // L 1 CPI 0.5

  // make mask of drgb values where d == 255
  __m256i src_depth_255_mask = _mm256_cmpeq_epi32(src_depth_unbiased_32, _mm256_set1_epi32(255)); // L 1 CPI 0.5 (depending on what compiler does with set1)

  // if all drgb values have d == 255 then quit because all are transparent
  if (_mm256_testc_si256(src_depth_255_mask, _mm256_set1_epi64x(-1))) // L 3 CPI 1
  {
    cout << "all src_depth == 255" << endl;
    return;
  }

  // split _m256 depth into 2 x _m128 depth to use _mm_packs_epi32
  __m128i src_depth_unbiased_32_1 = _mm256_extracti128_si256(src_depth_unbiased_32, 1); // L 3 CPI 1
  __m128i src_depth_unbiased_32_0 = _mm256_castsi256_si128(src_depth_unbiased_32); // no-op

  // (int16_t)(src_drgv >> 24)
  __m128i src_depth_unbiased_16 = _mm_packs_epi32(src_depth_unbiased_32_0, src_depth_unbiased_32_1); // L 1 CPI 1

  print_u16("src_depth_unbiased_16", src_depth_unbiased_16);

  // clamp(-32768, 32767, srcBias + src_depth_unbiased_16)
  // NOTE: can compute _mm_set1_epi16 ahead of time to avoid latency
  __m128i src_depth_biased = _mm_adds_epi16(src_depth_unbiased_16, _mm_set1_epi16(srcBias)); // L 1 CPI 0.5 + L 3 CPI 1

  print_i16("src_depth_biased", src_depth_biased);

}