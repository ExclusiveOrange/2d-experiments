#pragma once

// TODO: wide character support will require extra work for conversion

#include <sstream>

namespace detail::toString
{
  extern thread_local std::ostringstream ss;
}

template< class ... Ts >
std::string
toString( const Ts &...vs )
{
  using namespace detail::toString;
  ss.str( "" );
  ss.clear();
  return ((ss << vs), ..., ss.str());
}
