#include <iostream>
#include <filesystem>
#include <functional>
#include <optional>
#include <utility>

#include <SDL.h>
#include <SDL_image.h>

#include "function_traits.hpp"
#include "toString.hpp"

namespace
{
  using errmsg = std::optional<std::string>;

  const std::filesystem::path ASSETS = "assets";
  const std::filesystem::path ASSETS_IMAGES = ASSETS / "images";

  struct Images
  {
    static constexpr const char *test1_file = "2-1 terrain tile 1.png";
    SDL_Surface *test1{};

    ~Images()
    {
      if (test1)
        SDL_FreeSurface(test1);
    }
  };

  errmsg loadImage(const char *filename, SDL_Surface **dest)
  {
    std::filesystem::path path = ASSETS_IMAGES / filename;

    if (SDL_Surface *maybeSurface = IMG_Load(path.string().c_str()))
      return (*dest = maybeSurface, std::nullopt);

    return toString("IMG_Load failed! SDL_Error: ", SDL_GetError());
  }

  errmsg
  withImages(Function<errmsg(const Images &)> auto &&f)
  {
    errmsg errormsg{};
    Images images{};
    bool allImagesLoaded = !(errormsg = loadImage(Images::test1_file, &images.test1));

    if (allImagesLoaded)
      return f(images);

    return errormsg;
  }

  errmsg
  withSdl(Function<errmsg()> auto &&f)
  {
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
      return toString("SDL_Init failed! SDL_Error: ", SDL_GetError());

    errmsg errormsg = f();
    SDL_Quit();
    return errormsg;
  }

  errmsg
  withSdlWindow(int width, int height, Function<errmsg(SDL_Window *)> auto &&f)
  {
    SDL_Window *sdlWindow =
        SDL_CreateWindow(
            "SDL Tutorial",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            width, height,
            SDL_WINDOW_SHOWN);

    if (sdlWindow == nullptr)
      return toString("SDL_CreateWindow failed! SDL_Error: ", SDL_GetError());

    errmsg errormsg = f(sdlWindow);
    SDL_DestroyWindow(sdlWindow);
    return errormsg;
  }

  errmsg
  runWithWindowAndImages(SDL_Window *sdlWindow, const Images &images)
  {
    auto surface = SDL_GetWindowSurface(sdlWindow);

    SDL_BlitSurface(images.test1, nullptr, surface, nullptr);
    SDL_UpdateWindowSurface(sdlWindow);

    // DELETE
    SDL_Delay(2000);

    return {};
  }

  errmsg
  runWithWindow(SDL_Window *sdlWindow)
  {
    SDL_Surface *sdlSurface = SDL_GetWindowSurface(sdlWindow);

    // load screen I guess
    SDL_FillRect(sdlSurface, nullptr, SDL_MapRGB(sdlSurface->format, 255, 255, 255));
    SDL_UpdateWindowSurface(sdlWindow);

    return withImages([=](const Images &images)->errmsg{ return runWithWindowAndImages(sdlWindow, images); });
  }
}

int main(int argc, char *argv[])
{
  constexpr int width = 1920, height = 1080;

  errmsg errormsg = withSdl([=]()->errmsg{ return withSdlWindow(width, height, runWithWindow); });

  if (errormsg)
    std::cerr << "error: " << *errormsg << std::endl;

  return 0;
}
