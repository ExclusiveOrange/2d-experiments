#include <iostream>
#include <filesystem>
#include <functional>
#include <future>
#include <optional>
#include <utility>

#include <SDL.h>
#include <SDL_image.h>

#include "function_traits.hpp"
#include "NoCopyNoMove.hpp"
#include "toString.hpp"

namespace
{
  using errmsg = std::optional<std::string>;

  namespace defaults::window
  {
    constexpr const int width = 1920;
    constexpr const int height = 1080;
  }

  namespace paths
  {
    const std::filesystem::path assets = "assets";
    const std::filesystem::path images = assets / "images";
  }

  //======================================================================================================================
  // structs

  struct Images : NoCopyNoMove
  {
    static constexpr const char *test1File = "2-1 terrain tile 1.png";
    SDL_Surface *test1{};

    ~Images()
    {
      SDL_FreeSurface(test1);
    }
  };

  //======================================================================================================================
  // acquire* functions
  // These should acquire a resource, call a provided function, then release the resource.
  // The purpose is to simplify and safen resource management.

  errmsg
  acquireImages(const std::filesystem::path &imagesPath, Function<errmsg(const Images &)> auto &&then)
  {
    auto loadImage = [&imagesPath](const char *filename, SDL_Surface **dest) -> errmsg
    {
      std::filesystem::path path{imagesPath / filename};

      SDL_Surface *maybeSurface = IMG_Load(path.string().c_str());

      if (!maybeSurface)
        return toString("IMG_Load failed: ", SDL_GetError());

      *dest = maybeSurface;

      return {};
    };

    errmsg errormsg{};
    Images images;

    bool allLoaded =
      !(errormsg = loadImage(Images::test1File, &images.test1));

    if (allLoaded)
      then(images);

    return errormsg;
  }

  errmsg
  acquireSdl(Function<errmsg()> auto &&then)
  {
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
      return toString("SDL_Init failed! SDL_Error: ", SDL_GetError());

    errmsg errormsg = then();
    SDL_Quit();

    return errormsg;
  }

  errmsg
  acquireSdlWindow(int width, int height, Function<errmsg(SDL_Window *)> auto &&then)
  {
    SDL_Window *sdlWindow =
      SDL_CreateWindow(
        "SDL Tutorial",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_SHOWN);

    if (sdlWindow == nullptr)
      return toString("SDL_CreateWindow failed! SDL_Error: ", SDL_GetError());

    errmsg errormsg = then(sdlWindow);
    SDL_DestroyWindow(sdlWindow);

    return errormsg;
  }

  //======================================================================================================================
  // misc functions

  void
  showSplashScreen(SDL_Window *window)
  {
    SDL_Surface *surface = SDL_GetWindowSurface(window);
    SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 255, 255, 255));
    SDL_UpdateWindowSurface(window);
  }

  //======================================================================================================================
  // use* functions
  // These use already-acquired resources either implicitly (sdl) or explicitly (sdlwindow).
  // The purpose is to contain operational logic.

  errmsg
  useSdlWindowAndImages(SDL_Window *window, const Images &images)
  {
    // for now just display an image on the window and wait a bit then quit

    SDL_BlitSurface(images.test1, nullptr, SDL_GetWindowSurface(window), nullptr);
    SDL_UpdateWindowSurface(window);
    SDL_Delay(2000);

    return {};
  }

  errmsg
  useSdlWindow(SDL_Window *window)
  {
    showSplashScreen(window);
    return acquireImages(paths::images, [=](const Images &images) {return useSdlWindowAndImages(window, images);});
  }

  errmsg
  useSdl()
  {
    return acquireSdlWindow(defaults::window::width, defaults::window::height, useSdlWindow);
  }
}

//======================================================================================================================

int main(int argc, char *argv[])
{
  if (errmsg errormsg = acquireSdl(useSdl))
    std::cerr << "error: " << *errormsg << std::endl;

  return 0;
}
