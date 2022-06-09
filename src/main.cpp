#include <iostream>
#include <filesystem>
#include <functional>
#include <future>
#include <optional>
#include <stdexcept>
#include <utility>

#include <glm/glm.hpp>
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

  namespace defaults::render
  {
    constexpr const float scale = 2.f;
    constexpr const char *scaleQuality = "nearest"; // see SDL_HINT_RENDER_SCALE_QUALITY in SDL_hints.h for other options
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

  struct ViewOfCpuFrameBuffer
  {
    uint32_t *image;
    int32_t *depth;
    int w, h;
  };

  struct ViewOfCpuImageWithDepth
  {
    uint32_t *drgb; // depth instead of alpha: 0-254 == test, 255 == cull (transparent)
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
    ViewOfCpuFrameBuffer dest, int destx, int desty,
    ViewOfCpuImageWithDepth src, int srcdepthbias)
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
        int sindex = srowstart + sx;
        uint32_t sdrgb = src.drgb[sindex];

        // source is transparent if source depth == 255, else test against dest
        if (sdrgb < 0xff000000)
        {
          int dindex = drowstart + sx;
          int ddepth = dest.depth[dindex];

          int sdepth = ((sdrgb & 0xff000000) >> 24) + srcdepthbias;

          if (sdepth < ddepth)
          {
            // depth test passed: overwrite dest image and dest depth
            uint32_t sargb = 0xff000000 | (sdrgb & 0xffffff);
            dest.image[dindex] = sargb;
            dest.depth[dindex] = sdepth;
          }
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
  struct RenderBufferTexture : NoCopyNoMove
  {
    // do not pass a temporary SdlRenderer
    RenderBufferTexture(SdlRenderer &&, int w, int h) = delete;

    RenderBufferTexture(const SdlRenderer &renderer, int w, int h)
      : renderer{renderer}
      , texture{SDL_CreateTexture(renderer.renderer, constants::sdl::renderFormat, SDL_TEXTUREACCESS_STREAMING, w, h)}
    {
      if (texture == nullptr)
        throw error("SDL_CreateTexture failed: ", SDL_GetError());
    }

    const SdlRenderer &renderer;
    SDL_Texture *const texture;

    ~RenderBufferTexture() {SDL_DestroyTexture(texture);}
  };

  // note: use std::optional to contain this if you want a replaceable object, then use std::optional.emplace(...)
  struct CpuFrameBuffer
  {
    CpuFrameBuffer(int w, int h)
      : image{std::make_unique<uint32_t[]>(w * h)}
      , depth{std::make_unique<int32_t[]>(w * h)}
      , w{w}, h{h} {}

    void useWith(Function<void(const ViewOfCpuFrameBuffer &)> auto &&f)
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
    FrameBuffers(SdlRenderer &&, int) = delete;

    // bigger scale -> fewer pixels and they are bigger
    FrameBuffers(const SdlRenderer &renderer, float scale)
      : renderer{renderer}
      , scale{scale}
    {
      if (scale <= 0.f)
        throw error("FrameBuffers constructor failed: invalid scale parameter (", scale, ") but must be > 0");
    }

    void renderWith(Function<void(const ViewOfCpuFrameBuffer &)> auto &&cpuRenderer)
    {
      allocateBuffersIfNecessary();
      cpuFrameBuffer->useWith(cpuRenderer);
      cpuFrameBuffer->useWith(
        [&](const ViewOfCpuFrameBuffer &imageWithDepth)
        {
          int imagePitch = imageWithDepth.w * sizeof(imageWithDepth.image[0]);

          if (SDL_UpdateTexture(renderBufferTexture->texture, nullptr, imageWithDepth.image, imagePitch))
            throw error("SDL_UpdateTexture failed: ", SDL_GetError());

          if (SDL_RenderCopy(renderer.renderer, renderBufferTexture->texture, nullptr, nullptr))
            throw error("SDL_RenderCopy failed: ", SDL_GetError());
        });
    }

    void present() {SDL_RenderPresent(renderer.renderer);}

  private:
    const SdlRenderer &renderer;
    const float scale;
    int scaledWidth{-1}, scaledHeight{-1};
    int lastWindowWidth{-1}, lastWindowHeight{-1};
    std::optional<RenderBufferTexture> renderBufferTexture;
    std::optional<CpuFrameBuffer> cpuFrameBuffer;

    void allocateBuffers()
    {
      renderBufferTexture.emplace(renderer, scaledWidth, scaledHeight);
      cpuFrameBuffer.emplace(scaledWidth, scaledHeight);
    }

    void allocateBuffersIfNecessary()
    {
      SDL_Surface *windowSurface = SDL_GetWindowSurface(renderer.window.window);

      if (lastWindowWidth != windowSurface->w || lastWindowHeight != windowSurface->h)
      {
        (lastWindowWidth = windowSurface->w, lastWindowHeight = windowSurface->h);
        (scaledWidth = windowSurface->w / scale, scaledHeight = windowSurface->h / scale);
        allocateBuffers();
      }
    }
  };

  //======================================================================================================================
  // TEMPORARY maybe, for rendering tiles / sprites for testing since I can't find a good ready-made application
  //   that lets me create a sprite with alpha channel as depth; maybe it can be done with Blender...

  struct Ray
  {
    glm::vec3 origin, unitDirection;
  };

  struct Intersection
  {
    glm::vec3 position;
    glm::vec3 normal;
    float distance;
  };

  struct Sphere
  {
    Sphere(glm::vec3 center, float radius)
      : center{center}, sqradius{radius * radius} {}

    std::optional<Intersection>
    nearestIntersection(const Ray &ray) const
    {
      // ray is ray.origin + d * ray.unitDirection = x
      // sphere is |x - center| = radius
      // then intersections satisfy |(ray.origin + d * ray.unitDirection) - center| = radius
      // solve for d

      auto square = [](auto x) {return x * x;};

      const glm::vec3 originMinusCenter = ray.origin - center;
      const float ray_dot_originMinusCenter = glm::dot(ray.unitDirection, originMinusCenter);

      float insideRadical = square(ray_dot_originMinusCenter) - glm::dot(originMinusCenter, originMinusCenter) + sqradius;

      if (insideRadical <= 0) // no intersection or exactly one glancing intersection (which I ignore)
        return std::nullopt;

      // two intersections; want to pick most backward intersection unless behind ray origin
      float sqrtInsideRadical = glm::sqrt(insideRadical);
      float outsideRadical = -ray_dot_originMinusCenter;

      // most forward intersection
      float d1 = outsideRadical + sqrtInsideRadical;
      if (d1 <= 0) // most-forward intersection is behind ray origin
        return std::nullopt;

      // else most forward intersection is ahead of ray origin,
      // and most backward intersection is behind or ahead of ray origin
      float d0 = outsideRadical - sqrtInsideRadical;

      // ray origin is inside sphere so clamp intersection to ray origin
      if (d0 < 0.f)
        return Intersection{.position = ray.origin, .normal = ray.unitDirection, .distance = 0.f};

      // sphere is wholly ahead of ray origin and emits a surface intersection
      Intersection intersection{.position = ray.origin + d0 * ray.unitDirection, .distance = d0};
      intersection.normal = glm::normalize(intersection.position - center);
      return intersection;
    }

  private:
    const glm::vec3 center;
    const float sqradius;
  };
}

//======================================================================================================================

int main(int argc, char *argv[])
{
  try
  {
    Sdl sdl;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, defaults::render::scaleQuality);

    SdlWindow window{sdl, defaults::window::width, defaults::window::height};
    SdlRenderer renderer{window};
    FrameBuffers frameBuffers{renderer, defaults::render::scale};

    //----------------------------------------------------------------------------------------------------------------------
    // TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING
    auto testRenderer =
      [](const ViewOfCpuFrameBuffer &imageWithDepth)
      {
        for (int y = 0; y < imageWithDepth.h; ++y)
          for (int x = 0; x < imageWithDepth.w; ++x)
          {
            uint32_t p = (0xFF000000) | ((x & y & 255) << 16) | ((x & ~y & 255) << 8) | (~x & ~y & 255);
            imageWithDepth.image[y * imageWithDepth.w + x] = p;
          }
      };

    auto renderSphere =
      [](const ViewOfCpuFrameBuffer &imageWithDepth)
      {
        constexpr const float viewAngle = glm::radians(30.f);
        constexpr const float scale = 6.f;

        // right: +x
        // forward: +y
        // up: +z

        constexpr const glm::vec3 right{1.f, 0.f, 0.f};
        constexpr const glm::vec3 left = -right;

        constexpr const glm::vec3 forward{0.f, 1.f, 0.f};
        constexpr const glm::vec3 backward = -forward;

        constexpr const glm::vec3 up{0.f, 0.f, 1.f};
        constexpr const glm::vec3 down = -up;

        // TODO: not sure if SDL image will be upside-down or not so may have to reverse some things here

        const Sphere sphere{glm::vec3{0.f}, 40.f};
        constexpr const glm::vec3 lightPosition{right * 100.f /*+ up * 100.f*/ + backward * 100.f};
        constexpr const glm::vec3 lightIntensity{1.f};

        // TODO: figure out exactly correct position based on angle and distance from origin
        glm::vec3 centerRayOrigin{backward * 100.f + up * 55.f};
        glm::vec3 unitRayDirection{glm::normalize(forward * glm::cos(viewAngle) + down * glm::sin(viewAngle))};
        glm::vec3 unitRelativeUp{glm::normalize(glm::cross(unitRayDirection, right))};
        glm::vec3 xStep = right / scale;
        glm::vec3 yStep = unitRelativeUp / scale;

        Ray ray{.unitDirection= unitRayDirection};

        // TODO: might need to adjust position by half a pixel one way or the other
        for (int y = 0; y < imageWithDepth.h; ++y)
        {
          glm::vec3 yOffset = (imageWithDepth.h / -2.f + y + 0.5f) * yStep;

          for (int x = 0; x < imageWithDepth.w; ++x)
          {
            glm::vec3 xOffset = (imageWithDepth.w / -2.f + x + 0.5f) * xStep;
            ray.origin = centerRayOrigin + yOffset + xOffset;

            uint8_t distance = 255;

            glm::vec3 color{};

            if (std::optional<Intersection> i = sphere.nearestIntersection(ray))
            {
              glm::vec3 directionToLight{glm::normalize(lightPosition - i->position)};
              color = lightIntensity * glm::dot(i->normal, directionToLight);
              distance = (uint8_t)i->distance;
            }

            glm::vec3 clampedColor{glm::clamp(color, glm::vec3{0.f}, glm::vec3{1.f})};
            glm::vec3 scaledColor{clampedColor * 255.f};
            uint32_t pixel = 0xff000000 | (uint32_t(scaledColor.x) << 16) | (uint32_t(scaledColor.y) << 8) | uint32_t(scaledColor.z);

            int dindex = y * imageWithDepth.w + x;
            imageWithDepth.image[dindex] = pixel;
            imageWithDepth.depth[dindex] = distance;
          }
        }
      };

//    frameBuffers.renderWith(testRenderer);
    frameBuffers.renderWith(renderSphere);
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
