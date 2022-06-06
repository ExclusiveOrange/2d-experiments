#include <iostream>
#include <optional>
#include <type_traits>
#include <utility>

#include <SDL.h>

#include "Func.hpp"
#include "toString.hpp"

namespace
{
  namespace imageFilenames
  {
    namespace terrainTiles
    {
      constexpr const char *test1 = "2-1 terrain tile 1.png";
    }
  }

  std::optional<std::string>
  withSdlWindow(int width, int height, Func<SDL_Window *, std::optional<std::string>> auto &&run)
  {
    std::optional<std::string> errormsg{};

    if(SDL_Init(SDL_INIT_VIDEO) < 0)
      errormsg = toString("SDL_Init failed! SDL_Error: ", SDL_GetError());
    else
    {
      SDL_Window *sdlWindow =
          SDL_CreateWindow(
              "SDL Tutorial",
              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
              width, height,
              SDL_WINDOW_SHOWN);

      if(sdlWindow == nullptr)
        errormsg = toString("SDL_CreateWindow failed! SDL_Error: ", SDL_GetError());
      else
      {
        errormsg = run(sdlWindow);
        SDL_DestroyWindow(sdlWindow);
      }

      SDL_Quit();
    }

    return errormsg;
  }

  std::optional<std::string>
  withImages()
  {

  }

  std::optional<std::string>
  run(SDL_Window *sdlWindow)
  {
    SDL_Surface *sdlSurface = SDL_GetWindowSurface(sdlWindow);

    SDL_FillRect(sdlSurface, nullptr, SDL_MapRGB(sdlSurface->format, 255, 255, 255));

    SDL_UpdateWindowSurface(sdlWindow);
    SDL_Delay(2000);

    return std::nullopt;
  }
}

int main(int argc, char *argv[])
{
  constexpr int width = 1920, height = 1080;

  if(std::optional<std::string> errormsg = withSdlWindow(width, height, run))
    std::cerr << "error: " << *errormsg << std::endl;

  return 0;
}
