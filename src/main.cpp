#include <iostream>
#include <optional>
#include <utility>

#include <SDL.h>

#include "toString.hpp"

namespace
{
  template< class F >
  std::optional<std::string>
  withSdlWindow( int width, int height, F &&f )requires std::is_invocable_v<F, SDL_Window *>
  {
    std::optional<std::string> errormsg{};

    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
      errormsg = toString( "SDL_Init failed! SDL_Error: ", SDL_GetError());
    else
    {
      SDL_Window *sdlWindow =
          SDL_CreateWindow(
              "SDL Tutorial",
              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
              width, height,
              SDL_WINDOW_SHOWN );

      if( sdlWindow == nullptr )
        errormsg = toString( "SDL_CreateWindow failed! SDL_Error: ", SDL_GetError());
      else
      {
        f( sdlWindow );
        SDL_DestroyWindow( sdlWindow );
      }

      SDL_Quit();
    }

    return errormsg;
  }

  void run( SDL_Window *sdlWindow )
  {
    SDL_Surface *sdlSurface = SDL_GetWindowSurface( sdlWindow );

    SDL_FillRect( sdlSurface, nullptr, SDL_MapRGB( sdlSurface->format, 255, 255, 255 ));

    SDL_UpdateWindowSurface( sdlWindow );
    SDL_Delay( 2000 );
  }
}

int main( int argc, char *argv[] )
{
  constexpr int width = 1920, height = 1080;

  if( std::optional<std::string> errormsg = withSdlWindow( width, height, run ))
    std::cerr << "error: " << *errormsg << std::endl;

  return 0;
}
