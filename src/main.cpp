#include <iostream>
#include <filesystem>
#include <optional>
#include <type_traits>
#include <utility>

#include <SDL.h>

#include "func.hpp"
#include "toString.hpp"

namespace
{
  using errmsg = std::optional<std::string>;

  namespace imageFilenames
  {
    namespace terrainTiles
    {
      constexpr const char *test1 = "2-1 terrain tile 1.png";
    }
  }

  struct Images
  {
    SDL_Surface *test1;

    ~Images()
    {
      SDL_FreeSurface(test1);
    }
  };

  errmsg
  withSdlWindow(int width, int height, func<SDL_Window *,errmsg> auto &&f)
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
        errormsg = f(sdlWindow);
        SDL_DestroyWindow(sdlWindow);
      }

      SDL_Quit();
    }

    return errormsg;
  }

  errmsg
  withImages(func<const Images &, errmsg> auto &&f)
  {
    // TODO
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
