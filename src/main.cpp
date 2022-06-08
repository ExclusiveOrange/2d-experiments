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
  namespace constants::sdl
  {
    constexpr const Uint32 renderFormat = SDL_PIXELFORMAT_ARGB8888;
  }

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
  acquireCpuImageWithDepth(int width, int height, Function<errmsg(const CpuImageWithDepth &)> auto &&useCpuImageWithDepth)
  {
    CpuImageWithDepth buffer{};

    buffer.image = new uint32_t[width * height];
    buffer.depth = new int32_t[width * height];
    buffer.w = width;
    buffer.h = height;

    errmsg errormsg = useCpuImageWithDepth(buffer);

    delete buffer.depth;
    delete buffer.image;

    return errormsg;
  }

  errmsg
  acquireImages(const std::filesystem::path &imagesPath, Function<errmsg(const Images &)> auto &&useImages)
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
      useImages(images);

    return errormsg;
  }

  errmsg
  acquireRenderTexture(SDL_Renderer *renderer, int width, int height, Function<errmsg(SDL_Texture *)> auto &&useRenderTexture)
  {
    SDL_Texture *renderTexture = SDL_CreateTexture(renderer, constants::sdl::renderFormat, SDL_TEXTUREACCESS_STREAMING, width, height);

    if (renderTexture == nullptr)
      return toString("SDL_CreateTexture failed: ", SDL_GetError());

    errmsg errormsg = useRenderTexture(renderTexture);
    SDL_DestroyTexture(renderTexture);

    return errormsg;
  }

  errmsg
  acquireSdl(Function<errmsg()> auto &&useSdl)
  {
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
      return toString("SDL_Init failed! SDL_Error: ", SDL_GetError());

    errmsg errormsg = useSdl();
    SDL_Quit();

    return errormsg;
  }

  errmsg
  acquireSdlRenderer(SDL_Window *window, Function<errmsg(SDL_Renderer *)> auto &&useSdlRenderer)
  {
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (renderer == nullptr)
      return toString("SDL_CreateRenderer failed: ", SDL_GetError());

    errmsg errormsg = useSdlRenderer(renderer);
    SDL_DestroyRenderer(renderer);

    return errormsg;
  }

  errmsg
  acquireSdlWindow(int width, int height, Function<errmsg(SDL_Window *)> auto &&useSdlWindow)
  {
    SDL_Window *sdlWindow =
      SDL_CreateWindow(
        "SDL Tutorial",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_SHOWN);

    if (sdlWindow == nullptr)
      return toString("SDL_CreateWindow failed! SDL_Error: ", SDL_GetError());

    errmsg errormsg = useSdlWindow(sdlWindow);
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

    SDL_Surface *windowSurface = SDL_GetWindowSurface(window);

    int windowWidth = windowSurface->w;
    int windowHeight = windowSurface->h;

    return acquireSdlRenderer(
      window,
      [=](SDL_Renderer *sdlRenderer) -> errmsg
      {
        return acquireRenderTexture(
          sdlRenderer, windowWidth, windowHeight,
          [=](SDL_Texture *renderTexture) -> errmsg
          {
            return acquireCpuImageWithDepth(
              windowWidth, windowHeight,
              [=](const CpuImageWithDepth &cpuImageWithDepth) -> errmsg
              {
                int imagePitch = cpuImageWithDepth.w * sizeof(cpuImageWithDepth.image[0]);

                // test render
                for (int y = 0; y < cpuImageWithDepth.h; ++y)
                  for (int x = 0; x < cpuImageWithDepth.w; ++x)
                  {
                    uint32_t p = (0xFF000000) | ((x & y & 255) << 16) | ((x & ~y & 255) << 8) | (~x & ~y & 255);
                    cpuImageWithDepth.image[y * cpuImageWithDepth.w + x] = p;
                  }

                if (SDL_UpdateTexture(renderTexture, nullptr, cpuImageWithDepth.image, imagePitch))
                  return toString("SDL_UpdateTexture failed: ", SDL_GetError());

                if (SDL_RenderCopy(sdlRenderer, renderTexture, nullptr, nullptr))
                  return toString("SDL_RenderCopy failed: ", SDL_GetError());

                SDL_RenderPresent(sdlRenderer);

                SDL_Delay(5000);

                return {};
              });
          });
      });

//    SDL_RenderCopy()


//    SDL_BlitSurface(images.test1, nullptr, SDL_GetWindowSurface(window), nullptr);
//    SDL_UpdateWindowSurface(window);
//    SDL_Delay(2000);
//
//    return {};
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
