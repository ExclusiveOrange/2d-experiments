#pragma once

#if defined(__GCC__) || defined(__clang__)

#include <cstdint>
#include <type_traits>

//----------------------------------------------------------------------------------------------------------------------
// is_gcc_vector

// thanks: https://stackoverflow.com/a/69967239
template<class T>
struct is_gcc_vector
{
  static std::false_type test(...);

  template<class B>
  static auto test(B) -> decltype(__builtin_convertvector(std::declval<B>(), B), std::true_type{});

  static constexpr bool value = decltype(test(std::declval<T>()))::value;
};

template<class T>
concept GccVector = is_gcc_vector<T>::value;

//----------------------------------------------------------------------------------------------------------------------
// type promotion / demotion

template<class V>
struct uint_promotion;

template<>
struct uint_promotion<uint8_t> { using bigger = uint16_t; };

template<>
struct uint_promotion<uint16_t> { using bigger = uint32_t; };

template<>
struct uint_promotion<uint32_t> { using bigger = uint64_t; };

//----------------------------------------------------------------------------------------------------------------------
// gcc_vector_traits

template<class V>
struct gcc_vector_traits;

template<class T, size_t B>
struct gcc_vector_traits<T __attribute__ ((vector_size(B)))>
{
  using base_type = T;
  using bigger_type = typename uint_promotion<T>::bigger __attribute__ ((vector_size(B)));
  static constexpr size_t num_bytes = B;
};

//----------------------------------------------------------------------------------------------------------------------
// gcc vector aliases (not exhaustive)

using uint8_16 = uint8_t __attribute__ ((vector_size(16))); // __m128i
using uint8_32 = uint8_t __attribute__ ((vector_size(32))); // __m256i

using uint16_8 = uint16_t __attribute__ ((vector_size(16))); // __m128i
using uint16_16 = uint16_t __attribute__ ((vector_size(32))); // __m256i

#else
#pragma warning(__FILE__ " was included but __GCC__ or __clang__ are not defined")
#endif
