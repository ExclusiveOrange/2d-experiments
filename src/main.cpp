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
  namespace constants::tile
  {
    constexpr const int width = 32;
    constexpr const int height = 16;
    constexpr const int centerx = width / 2;
    constexpr const int centery = height / 2;
  }

  namespace defaults::window
  {
    constexpr const int width = 1920;
    constexpr const int height = 1080;
  }

  namespace defaults::paths
  {
    const std::filesystem::path assets = "assets";
    const std::filesystem::path images = assets / "images";
  }

  //======================================================================================================================
  // error transmission

  using errmsg = std::optional<std::string>;

  //======================================================================================================================
  // structs

  struct CpuImageWithDepth
  {
    uint32_t *image;
    int32_t *depth;
    int w, h;
  };

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
  acquireCpuImageWithDepth(int width, int height, Function<errmsg(const CpuImageWithDepth&)> auto &&use)
  {
    CpuImageWithDepth buffer{};

    buffer.image = new uint32_t[width * height];
    buffer.depth = new int32_t[width * height];
    buffer.w = width;
    buffer.h = height;

    errmsg errormsg = use(buffer);

    delete buffer.depth;
    delete buffer.image;

    return errormsg;
  }

  errmsg
  acquireImages(const std::filesystem::path &imagesPath, Function<errmsg(const Images &)> auto &&use)
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
      use(images);

    return errormsg;
  }

  errmsg
  acquireSdl(Function<errmsg()> auto &&use)
  {
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
      return toString("SDL_Init failed! SDL_Error: ", SDL_GetError());

    errmsg errormsg = use();
    SDL_Quit();

    return errormsg;
  }

  errmsg
  acquireSdlWindow(int width, int height, Function<errmsg(SDL_Window *)> auto &&use)
  {
    SDL_Window *sdlWindow =
      SDL_CreateWindow(
        "SDL Tutorial",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_SHOWN);

    if (sdlWindow == nullptr)
      return toString("SDL_CreateWindow failed! SDL_Error: ", SDL_GetError());

    errmsg errormsg = use(sdlWindow);
    SDL_DestroyWindow(sdlWindow);

    return errormsg;
  }

  //======================================================================================================================
  // misc functions

  //
  void
  drawWithDepth(
    CpuImageWithDepth dest, int destx, int desty,
    CpuImageWithDepth src, int srcdepthbias)
  {
    // clip vertical
    int minsy = desty < 0 ? -desty : 0;
    int maxsy = (dest.h - desty) < src.h ? (dest.h - desty) : src.h;

    // clip horizontal
    int minsx = destx < 0 ? -destx : 0;
    int maxsx = (dest.w - destx) < src.w ? (dest.w - destx) : src.w;

    // for each non-clipped pixel in source...
    for (int sy = minsy; sy < maxsy; ++sy)
    {
      int drowstart = (desty + sy) * dest.w + destx;
      int srowstart = sy * src.w;

      for (int sx = minsx; sx < maxsx; ++sx)
      {
        int dindex = drowstart + sx;
        int sindex = srowstart + sx;

        // test depth
        int ddepth = dest.depth[dindex];
        int sdepth = src.depth[sindex] + srcdepthbias;

        if (sdepth < ddepth)
        {
          // depth test passed: overwrite dest image and dest depth
          dest.image[dindex] = src.image[sindex];
          dest.depth[dindex] = sdepth;
        }
      }
    }
  }

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
    return acquireImages(defaults::paths::images, [=](const Images &images) {return useSdlWindowAndImages(window, images);});
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
