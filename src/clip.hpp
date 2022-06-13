#pragma once

#include <concepts>

template<class T> requires std::is_integral_v<T> || std::is_floating_point_v<T>
constexpr T
clipMin(T destx, T destw, T srcw) {return destx < 0 ? -destx : 0;}

template<class T> requires std::is_integral_v<T> || std::is_floating_point_v<T>
constexpr T
clipMax(T destx, T destw, T srcw) {return (destw - destx) < srcw ? (destw - destx) : srcw;}
