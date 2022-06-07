#pragma once

#include <functional>
#include <utility>

struct Destroyer
{
  using fn_t = std::function<void( void )>;

  Destroyer() = default;

  explicit
  Destroyer( fn_t &&fnOnDestroy )
      : fnOnDestroy{ std::move( fnOnDestroy ) } {}

  Destroyer( Destroyer &&other ) noexcept
      : fnOnDestroy{ std::exchange( other.fnOnDestroy, {} ) } {}

  ~Destroyer()
  {
    if( fnOnDestroy )
      fnOnDestroy();
  }

  Destroyer &operator=( Destroyer &&other ) noexcept
  {
    if( fnOnDestroy )
      fnOnDestroy();

    fnOnDestroy = std::exchange( other.fnOnDestroy, {} );

    return *this;
  }

private:
  fn_t fnOnDestroy;
};
