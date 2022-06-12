#pragma once

#include <concepts>

template<class T> requires std::integral<T> || std::floating_point<T>
constexpr T
clipMin(T destx, T destw, T srcw) {return destx < 0 ? -destx : 0;}

template<class T> requires std::integral<T> || std::floating_point<T>
constexpr T
clipMax(T destx, T destw, T srcw) {return (destw - destx) < srcw ? (destw - destx) : srcw;}
