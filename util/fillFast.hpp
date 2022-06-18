#pragma once

#include <cstdint>

#ifdef __AVX2__

#include <immintrin.h>

void fillFast(uint32_t *dst, size_t n, uint32_t val)
{
  // AVX version which uses streaming store which tells the CPU to bypass the write cache.
  // For some reason this is dramatically faster than using non-streaming store.

  // unaligned head
  {
    // align size to 32 bytes
    size_t headN = (size_t)((uintptr_t)dst & 31) / sizeof(uint32_t);
    headN = headN < n ? headN : n;

    for (size_t i = 0; i < headN; ++i)
      _mm_stream_si32((int *)(dst + i), val);

    if (headN >= n)
      return;

    dst += headN;
    n -= headN;
  }

  // aligned body
  constexpr size_t simdSize = 8;
  const size_t simdN = n - n % simdSize;

  __m256i val8 = _mm256_set1_epi32(val);
  for (size_t i = 0; i < simdN; i += simdSize)
    _mm256_stream_si256((__m256i *)(dst + i), val8);

  // too-small tail
  for (size_t i = simdN; i < n; ++i)
    _mm_stream_si32((int *)(dst + i), val);
}

void fillFast(int16_t *dst, size_t n, int16_t val)
{
  // AVX version which uses streaming store which tells the CPU to bypass the write cache.
  // For some reason this is dramatically faster than using non-streaming store.

  // unaligned head
  {
    // align size to 32 bytes
    size_t headN = (size_t)((uintptr_t)dst & 31) / sizeof(uint16_t);
    headN = headN < n ? headN : n;

    for (size_t i = 0; i < headN; ++i)
      dst[i] = val;

    if (headN >= n)
      return;

    dst += headN;
    n -= headN;
  }

  // aligned body
  constexpr size_t simdSize = 16;
  const size_t simdN = n - n % simdSize;

  __m256i val8 = _mm256_set1_epi16(val);
  for (size_t i = 0; i < simdN; i += simdSize)
    _mm256_stream_si256((__m256i *)(dst + i), val8);

  // too-small tail
  for (size_t i = simdN; i < n; ++i)
    dst[i] = val;
}

#else // not __AVX2__

#include <algorithm>

void fillFast(uint32_t *dst, size_t n, uint32_t val)
{
  std::fill_n(dst, n, val);
}

void fillFast(int16_t *dst, size_t n, int16_t val)
{
  std::fill_n(dst, n, val);
}

#endif
