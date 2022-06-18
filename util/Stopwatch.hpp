#pragma once

#include <chrono>

struct Stopwatch
{
  using clock = std::chrono::high_resolution_clock;
  
  clock::time_point t;
  clock::duration d{};
  
  void start() {t = clock::now();}
  void stop() {d = clock::now() - t;}
  auto millis() const {return (1.0 / 1000.0) * (double)std::chrono::duration_cast<std::chrono::microseconds>(d).count();}
};

