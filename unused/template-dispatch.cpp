// 2022.06.18 Atlee
// Wrote this to test compile-time template specialization existence and error messages.
//
// The concept AdderExists below is used in the addArrays function below to ensure
// that the template class parameter has specific specializations.
//
// This solution was inspired by wanting to support a variety of pixel blending functions,
// each potentially with SIMD specializations where supported by the compiler.
//
// For example if I have an image blending function that takes a pixel blending function
// as a parameter, then for performance it is desirable to have pixel blending function
// passed in such a way to allow the compiler to inline the function.
// This can be done with normal templates, but the error messages are not helpful when something
// is wrong.
// But if the compiler supports SIMD then a different pixel blending function should be used
// to take advantage of that.
//
// This solution allows the point of use to specify a template struct as a template parameter
// to the image blending function, where the template struct may have specializations for SIMD,
// and the point of use does not need to know that.

#include <cstdint>
#include <iostream>

using namespace std;

using uint32_8 = uint32_t __attribute__ ((vector_size(8 * sizeof(uint32_t))));

template<class T>
struct Adder;

template<>
struct Adder<uint32_t>
{
  static
  uint32_t add(uint32_t a, uint32_t b)
  {
    return a + b;
  }
};

#if defined(__GCC__) || defined(__clang__)
template<>
struct Adder<uint32_8>
{
  static
  uint32_8 add(uint32_8 a, uint32_8 b)
  {
    return a + b;
  }
};
#endif

template<template<class T> class Adder, class T>
concept AdderExists =
requires(Adder<T> &&s, T &&a, T &&b)
{
  sizeof(Adder<T>); // fails if Adder<T> does not exist
  {Adder<T>::add(a, b)} -> std::same_as<T>; // fails if Adder<T> does not have method T add(T, T)
};

#if defined(__GCC__) || defined(__clang__)
template<template<class> class Adder>
void addArrays(uint32_t *__restrict dst, uint32_t *__restrict a, uint32_t *__restrict b, size_t n)
requires AdderExists<Adder, uint32_t> && AdderExists<Adder, uint32_8>
{
  for (; n > 8; n -= 8, dst += 8, a += 8, b += 8)
  {
    uint32_8 _a = *(uint32_8 *)a;
    uint32_8 _b = *(uint32_8 *)b;
    *(uint32_8 *)dst = Adder<uint32_8>::add(_a, _b);
  }

  for (; n > 0; --n, ++dst, ++a, ++b)
    *dst = Adder<uint32_t>::add(*a, *b);
}
#else
template<template<class> class Adder>
void addArrays(uint32_t *__restrict dst, uint32_t *__restrict a, uint32_t *__restrict b, size_t n)
requires AdderExists<Adder, uint32_t>
{
  for (; n > 0; --n, ++dst, ++a, ++b)
    *dst = AddFunctor<uint32_t>::add(*a, *b);
}
#endif

int main()
{
  constexpr size_t N = 15;
  uint32_t a[N], b[N], dst[N];

  for (int i = 0; i < N; ++i)
  {
    a[i] = i;
    b[i] = i * 1000;
    dst[i] = 0;
  }

  addArrays<Adder>(dst, a, b, N);

  cout << "results:\n";
  for (int i = 0; i < N; ++i)
    cout << ((i != 0) ? ", " : "") << dst[i];
  cout << endl;

  return 0;
}
