#pragma once

// inherit to inhibit copy and move
struct NoCopyNoMove
{
  NoCopyNoMove() = default;

  NoCopyNoMove(const NoCopyNoMove&) = delete;
  NoCopyNoMove(NoCopyNoMove&&) = delete;

  NoCopyNoMove& operator =(const NoCopyNoMove&) = delete;
  NoCopyNoMove& operator =(NoCopyNoMove&&) = delete;
};