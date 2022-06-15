//
// Created by Atlee on 6/14/2022.
//

#ifdef __AVX2__
#pragma message("AVX2 detected; assuming SSE4.2")
#include <immintrin.h>

#endif

#include <bitset>
#include <iostream>

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

  __m128i u32a = _mm_loadu_si128((__m128i *)&u32vs[0]); // latency 1, throughput 0.5
  __m128i u32b = _mm_loadu_si128((__m128i *)&u32vs[4]); // latency 1, throughput 0.5
  __m128i s16 = _mm_packs_epi32(u32a, u32b); // latency 1, throughput 0.5
  _mm_storeu_si128((__m128i *)i16check_8, s16); // latency 1, throughput 0.5
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

  __m128i a = _mm_loadu_si128((__m128i *)i16_a);
  __m128i b = _mm_loadu_si128((__m128i *)i16_b);

  print_i16("a", a);
  print_i16("b", b);

  __m128i altb = _mm_cmpgt_epi16(b, a);

  if (_mm_testz_si128(altb, _mm_set1_epi64x(-1)))
  {
    cout << "all a >= b" << endl;
    return;
  }

  __m256i altb_32 = _mm256_cvtepi16_epi32(altb);

  print_u32("altb_32", altb_32);

  __m256i c = _mm256_loadu_si256((__m256i*)u32_c);
  __m256i d = _mm256_loadu_si256((__m256i*)u32_d);

  __m256i c_or_d = _mm256_blendv_epi8(d, c, altb_32);
  _mm256_storeu_si256((__m256i*)u32_d, c_or_d);

  print("c", u32_c);
  print("d", u32_d);
}