//
// Created by Atlee on 6/14/2022.
//

#ifdef __AVX2__
#pragma message("AVX2 detected; assuming SSE4.2")
#include <immintrin.h>

#endif

#include <iostream>

using namespace std;

static
void
print(const char *name, auto &&a)
{
  cout << name << "{";
  bool first = true;
  for(auto x : a)
  {
    if (first)
      first = !first;
    else
      cout << ", ";
    cout << x;
  }
  cout << "}" << endl;
}

void simdtest()
{

  // GOAL:
  // load 8 x uint32 values
  // convert to 8 x int16 values
  // store 8 x int16 values

  uint32_t u32vs[8];
  uint32_t u32check[8];
  uint16_t u16check_16[16];
  uint16_t u16check_8[8];

  for (int i = 0; i < 8; ++i)
    u32vs[i] = i;

  __m256i a = _mm256_loadu_si256((__m256i*)u32vs);

  _mm256_storeu_si256((__m256i*)u32check, a);
  print("a", u32check);

  _mm256_storeu_si256((__m256i*)u16check_16, a);
  print("a as u16", u16check_16);

  __m256i shuffledlo = _mm256_shufflelo_epi16(a, _MM_SHUFFLE(0,0,2,0));
  // 0 1 _ _ _ _ _ _ 4 5 _ _ _ _ _ _
  _mm256_storeu_si256((__m256i*)u16check_16, shuffledlo);
  print("shuffledlo", u16check_16);

  __m256i shuffledhi = _mm256_shufflehi_epi16(a, _MM_SHUFFLE(0,0,2,0));
  // _ _ _ _ 2 3 _ _ _ _ _ _ 6 7 _ _
  _mm256_storeu_si256((__m256i*)u16check_16, shuffledhi);
  print("shuffledhi", u16check_16);

  __m256i blendedshuffled = _mm256_blend_epi32(shuffledlo, shuffledhi, _MM_SHUFFLE(1,0,1,0));
  _mm256_storeu_si256((__m256i*)u16check_16, blendedshuffled);
  print("blendedshuffled", u16check_16);

  __m256i blendedshuffledlo = _mm256_shuffle_epi32(blendedshuffled, _MM_SHUFFLE(0, 0, 2, 0));
  _mm256_storeu_si256((__m256i*)u16check_16, blendedshuffledlo);
  print("blendedshuffledlo", u16check_16);

  __m256i permuted = _mm256_permute4x64_epi64(blendedshuffledlo, _MM_SHUFFLE(0, 0, 2, 0));
  _mm256_storeu_si256((__m256i*)u16check_16, permuted);
  print("permuted", u16check_16);

  __m128i final16s = _mm256_extracti128_si256(permuted, 0);

  _mm_store_si128((__m128i*)u16check_8, final16s);

  //__m128i lo128 = _mm256_extracti128_si256(blendedshuffledlo, 0);
  //__m128i hi128 = _mm256_extracti128_si256(blendedshuffledlo, 1);
  //
  //_mm_storel_epi64((__m128i*)&u16check_8[0], lo128);
  //_mm_storel_epi64((__m128i*)&u16check_8[4], hi128);

  print("final u16check_8", u16check_8);


  cout << "simdtest()" << endl;

}