#pragma once

struct NoCopy
{
  NoCopy() = default;

  NoCopy(NoCopy&&) = default;
  NoCopy & operator=(NoCopy&&) = default;

  ~NoCopy() = default;
};
