#pragma once

struct NoCopyNoMove
{
  NoCopyNoMove() = default;

  NoCopyNoMove(const NoCopyNoMove&) = delete;
  NoCopyNoMove(NoCopyNoMove&&) = delete;
};