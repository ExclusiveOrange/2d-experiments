#pragma once

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

namespace gradient
{
  template<class V>
  std::function<V(float t)>
  makeGradient(std::vector<std::pair<float, V>> points)
  {
    if (points.size() <= 0)
      throw std::runtime_error("empty list passed to makeGradient");

    if (points.size() == 1)
      return [v = points.front().second](float) {return v;};

    std::sort(points.begin(), points.end(), [](const std::pair<float, V> &a, const std::pair<float, V> &b) {return a.first < b.first;});

    struct Point
    {
      V v;
      float t, recipDiffToNext;
    };

    std::vector<Point> pointsWithDiffs{points.size()};

    for (int i = 0; i < points.size() - 1; ++i)
    {
      float diff = points[i + 1].first - points[i].first;
      pointsWithDiffs[i] = Point{.v = points[i].second, .t = points[i].first, .recipDiffToNext = (diff != 0.f) ? 1.f / diff : 0.f};
    }

    return [points = std::move(pointsWithDiffs)]
      (float t) -> V
    {
      auto nextIt = std::upper_bound(
        points.begin(), points.end(),
        t,
        [](float t, const Point &point) {return t < point.t;});

      if (nextIt == points.begin())
        return points.front().v;

      if (nextIt == points.end())
        return points.back().v;

      Point a = *(nextIt - 1);
      V b = nextIt->v;

      return glm::mix(a.v, b, (t - a.t) * a.recipDiffToNext);
    };
  }
}
