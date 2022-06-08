#include <iostream>
#include <filesystem>
#include <functional>
#include <future>
#include <optional>
#include <stdexcept>
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

  template<class...Args>
  std::runtime_error
  error(Args &&...args) {return std::runtime_error(toString(std::forward<Args>(args)...));}

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
  // misc functions

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

  //======================================================================================================================
  // another attempt to get the right safety behavior I want before I get too far into this project

  struct Sdl : NoCopyNoMove
  {
    Sdl()
    {
      if (SDL_Init(SDL_INIT_VIDEO) < 0)
        throw error("SDL_Init failed! SDL_Error: ", SDL_GetError());
    }

    ~Sdl() {SDL_Quit();}
  };

  struct SdlWindow : NoCopyNoMove
  {
    // do not pass a temporary Sdl
    SdlWindow(Sdl &&, int w, int h) = delete;

    SdlWindow(const Sdl &, int w, int h)
      : window{
      SDL_CreateWindow(
        "SDL Tutorial",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        w, h,
        SDL_WINDOW_SHOWN)}
    {
      if (window == nullptr)
        throw error("SDL_CreateWindow failed! SDL_Error: ", SDL_GetError());
    }

    SDL_Window *const window;

    ~SdlWindow() {SDL_DestroyWindow(window);}
  };

  struct SdlRenderer : NoCopyNoMove
  {
    // do not pass a temporary SdlWindow
    SdlRenderer(SdlWindow &&) = delete;

    SdlRenderer(const SdlWindow &window)
      : window{window}
      , renderer{SDL_CreateRenderer(window.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)}
    {
      if (renderer == nullptr)
        throw error("SDL_CreateRenderer failed: ", SDL_GetError());
    }

    const SdlWindow &window;
    SDL_Renderer *const renderer;

    ~SdlRenderer() {SDL_DestroyRenderer(renderer);}
  };

  // note: use std::optional to contain this if you want a replaceable object, then use std::optional.emplace(...)
  struct RenderTexture : NoCopyNoMove
  {
    // do not pass a temporary SdlRenderer
    RenderTexture(SdlRenderer &&, int w, int h) = delete;

    RenderTexture(const SdlRenderer &renderer, int w, int h)
      : renderer{renderer}
      , texture{SDL_CreateTexture(renderer.renderer, constants::sdl::renderFormat, SDL_TEXTUREACCESS_STREAMING, w, h)}
    {
      if (texture == nullptr)
        throw error("SDL_CreateTexture failed: ", SDL_GetError());
    }

    const SdlRenderer &renderer;
    SDL_Texture *const texture;

    ~RenderTexture() {SDL_DestroyTexture(texture);}
  };

  struct CpuFrameBuffer
  {
    CpuFrameBuffer(int w, int h)
      : image{std::make_unique<uint32_t[]>(w * h)}
      , depth{std::make_unique<int32_t[]>(w * h)}
      , w{w}, h{h} {}

    void useWith(Function<void(const CpuImageWithDepth &)> auto &&f)
    {
      f({.image = image.get(), .depth = depth.get(), .w = w, .h = h});
    }

  private:
    const std::unique_ptr<uint32_t[]> image;
    const std::unique_ptr<int32_t[]> depth;
    const int w, h;
  };

  struct FrameBuffers : NoCopyNoMove
  {
    // do not pass a temporary SdlRenderer
    FrameBuffers(SdlRenderer &&) = delete;

    FrameBuffers(const SdlRenderer &renderer)
      : renderer{renderer} {}

    void renderWith(Function<void(const CpuImageWithDepth &)> auto &&cpuRenderer)
    {
      allocateBuffersIfNecessary();
      cpuFrameBuffer->useWith(cpuRenderer);
      cpuFrameBuffer->useWith(
        [&](const CpuImageWithDepth &imageWithDepth)
        {
          int imagePitch = imageWithDepth.w * sizeof(imageWithDepth.image[0]);

          if (SDL_UpdateTexture(renderTexture->texture, nullptr, imageWithDepth.image, imagePitch))
            throw error("SDL_UpdateTexture failed: ", SDL_GetError());

          if (SDL_RenderCopy(renderer.renderer, renderTexture->texture, nullptr, nullptr))
            throw error("SDL_RenderCopy failed: ", SDL_GetError());
        });
    }

    void present() {SDL_RenderPresent(renderer.renderer);}

  private:
    const SdlRenderer &renderer;
    int w{-1}, h{-1};
    std::optional<RenderTexture> renderTexture;
    std::optional<CpuFrameBuffer> cpuFrameBuffer;

    void allocateBuffers()
    {
      renderTexture.emplace(renderer, w, h);
      cpuFrameBuffer.emplace(w, h);
    }

    void allocateBuffersIfNecessary()
    {
      SDL_Surface *windowSurface = SDL_GetWindowSurface(renderer.window.window);

      if (w != windowSurface->w || h != windowSurface->h)
      {
        (w = windowSurface->w, h = windowSurface->h);
        allocateBuffers();
      }
    }
  };
}

//======================================================================================================================

int main(int argc, char *argv[])
{
  try
  {
    Sdl sdl;
    SdlWindow window{sdl, defaults::window::width, defaults::window::height};
    SdlRenderer renderer{window};
    FrameBuffers frameBuffers{renderer};

    //----------------------------------------------------------------------------------------------------------------------
    // TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING
    auto testRenderer =
      [](const CpuImageWithDepth &imageWithDepth)
      {
        for (int y = 0; y < imageWithDepth.h; ++y)
          for (int x = 0; x < imageWithDepth.w; ++x)
          {
            uint32_t p = (0xFF000000) | ((x & y & 255) << 16) | ((x & ~y & 255) << 8) | (~x & ~y & 255);
            imageWithDepth.image[y * imageWithDepth.w + x] = p;
          }
      };

    frameBuffers.renderWith(testRenderer);
    frameBuffers.present();

    SDL_Delay(3000);

    // TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING
    //----------------------------------------------------------------------------------------------------------------------

    // TODO: render loop: check for events, render to frame buffer, present frame buffer, etc.
  }
  catch (const std::exception &e)
  {
    std::cerr << "caught exception in main: " << e.what() << std::endl;
  }

  return 0;
}
