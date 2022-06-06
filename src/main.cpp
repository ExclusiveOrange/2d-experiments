#include <iostream>
#include <utility>

#include <SDL.h>

#include "ErrorOrSuccess.hpp"

using namespace error_or_success;

namespace
{
  ErrorOrSuccess init(int x)
  {
    return x < 0 ? Error("negative") : Success;
  }
}

int main( int argc, char *argv[] )
{
  init(1)
      .error( []( auto errormsg ) { std::cerr << "error: " << errormsg << std::endl; } )
      .success( [] { std::cout << "success" << std::endl; } );

  return 0;
}
