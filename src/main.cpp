// 2d-experiments with SDL
// 2022 Atlee Brink
// Experiments with 2d, probably CPU, rendering using SDL for window management.
// Thinking along the lines of orthographic top-down tiles and sprites,
// but I want to explore uncommon effects like depth buffering as well as
// more complex effects that would be difficult to generalize in a GPU shader.

// C++ standard library
#include <algorithm>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <functional>
#include <future>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

// third party
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL.h>
#include <SDL_image.h>

// this project
#include "CpuFrameBuffer.hpp"
#include "CpuImageWithDepth.hpp"
#include "drawWithDepth.hpp"
#include "drawWithoutDepth.hpp"
#include "raycasting.hpp"
#include "raycasting_shapes.hpp"

// utility
#include "glmprint.hpp"
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
    constexpr const char *title = "2d-experiments";
    constexpr const int width = 1200;
    constexpr const int height = 900;
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
  // performance measurement

  using clock = std::chrono::high_resolution_clock;

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
  // misc functions

  void
  setPlatformSpecificSdlHints()
  {
#ifdef APPLE
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"); // SDL tries to use Metal by default but it is catastrophically slow
#endif
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
    // TODO: figure out how to deal with Apple retina resolution,
    // where the SDL_CreateWindow actually takes width, height in points, not pixels,
    // so the size of everything is wrong.

    // do not pass a temporary Sdl
    SdlWindow(Sdl &&, int w, int h) = delete;

    SdlWindow(const Sdl &, int w, int h)
      : window{
      SDL_CreateWindow(
        defaults::window::title,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        w, h,
        //SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI)} // Apple retina displays have very high DPI so maybe we want to reduce it; or could change the render scale
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

  struct FrameBuffers : NoCopyNoMove
  {
    // do not pass a temporary SdlRenderer
    FrameBuffers(SdlRenderer &&, int) = delete;

    // bigger scale -> fewer pixels and they are bigger
    FrameBuffers(const SdlRenderer &renderer, float scale, bool flipVertical = true)
      : renderer{renderer}
      , scale{scale}
      , flip{flipVertical ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE}
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

          if (SDL_RenderCopyEx(renderer.renderer, renderBufferTexture->texture, nullptr, nullptr, 0.0, nullptr, flip))
            throw error("SDL_RenderCopyEx failed: ", SDL_GetError());
        });
    }

    void present() {SDL_RenderPresent(renderer.renderer);}

  private:
    const SdlRenderer &renderer;
    const float scale;
    const SDL_RendererFlip flip;

    int scaledWidth{-1}, scaledHeight{-1};
    int lastRendererWidth{-1}, lastRendererHeight{-1};
    std::optional<RenderBufferTexture> renderBufferTexture;
    std::optional<CpuFrameBuffer> cpuFrameBuffer;

    void allocateBuffers()
    {
      renderBufferTexture.emplace(renderer, scaledWidth, scaledHeight);
      cpuFrameBuffer.emplace(scaledWidth, scaledHeight);
    }

    void allocateBuffersIfNecessary()
    {
      int rendererWidth, rendererHeight;

      if (0 != SDL_GetRendererOutputSize(renderer.renderer, &rendererWidth, &rendererHeight))
        throw error("SDL_GetRendererOutputSize failed: ", SDL_GetError());

      if (lastRendererWidth != rendererWidth || lastRendererHeight != rendererHeight)
      {
        (lastRendererWidth = rendererWidth, lastRendererHeight = rendererHeight);
        (scaledWidth = rendererWidth / scale, scaledHeight = rendererHeight / scale);
        allocateBuffers();
      }
    }
  };
}

//======================================================================================================================

namespace testing
{
  class SpriteRenderer : NoCopyNoMove
  {
    glm::ivec2 spriteSize;
    CpuImageWithDepth spriteSphere, spriteQuad;

  public:
    SpriteRenderer(
      const raycasting::OrthogonalCamera &camera,
      int spriteWidth, int spriteHeight)
      : spriteSize{spriteWidth, spriteHeight}
      , spriteSphere{spriteWidth, spriteHeight}
      , spriteQuad{spriteWidth, spriteHeight}
    {
      using namespace raycasting;
      using namespace raycasting::shapes;

      const float sphereRadius = 30.f;
      const float depthRange = 128.f;

      Sphere sphere{glm::vec3{0.f}, sphereRadius};
      //QuadUp quad{glm::vec3{0.f}, glm::sqrt(2.f * spriteWidth * spriteWidth) * 0.25f};
      float side = glm::sqrt(2.f * spriteWidth * spriteWidth) * 0.3f;
      Quad quad{glm::vec3{0.f}, 0.5f * side * glm::normalize(3.f * forward + up), side * glm::normalize(2.f * right + up)};

      glm::vec3 minLight{0.2f, 0.15f, 0.1f};

      std::vector<DirectionalLight> directionalLights{DirectionalLight{glm::normalize(forward + 1.5f * down), glm::vec3{1.f, 1.f, 1.f}}};

      camera.render(
        spriteSphere.getUnsafeView(),
        sphere,
        minLight,
        &directionalLights[0],
        directionalLights.size());

      camera.render(
        spriteQuad.getUnsafeView(),
        quad,
        minLight,
        &directionalLights[0],
        directionalLights.size());
    }

    void render(const ViewOfCpuFrameBuffer &frameBuffer)
    {
      frameBuffer.clear(0xff000000, 0x7fff);

      const glm::ivec2 destCenter{frameBuffer.w / 2, frameBuffer.h / 2};
      const glm::ivec2 spacing{spriteSize.x, spriteSize.y / 2};
      const glm::ivec2 steps = 1 + glm::ivec2{frameBuffer.w, frameBuffer.h} / (2 * spacing);

      for (glm::ivec2 pos{-steps}; pos.y <= steps.y; ++pos.y)
        for (pos.x = -steps.x; pos.x <= steps.x; ++pos.x)
        {
          glm::ivec2 destPos = destCenter - spriteSize / 2 + pos * spacing;
          int depthBias = 2 * destPos.y;
          destPos.x += (pos.y & 1) * spacing.x / 2;

          drawWithDepth(
            frameBuffer,
            destPos.x, destPos.y,
            spriteSphere.getUnsafeView(),
            depthBias);

          drawWithDepth(
            frameBuffer,
            destPos.x, destPos.y,
            spriteQuad.getUnsafeView(),
            depthBias);
        }
    }
  };

  class TileRenderer : NoCopyNoMove
  {
    //static constexpr glm::ivec2 tileworldsize;
    const glm::ivec2 tileSizeWorld;
    const glm::ivec2 tileSizeScreen;
    const glm::imat2x2 tileSpacingScreen;

    const CpuImageWithDepth tile0;

  public:
    TileRenderer(
      const raycasting::OrthogonalCamera &camera,
      glm::ivec2 tileSizeWorld,
      glm::ivec2 tileSizeScreen,
      glm::imat2x2 tileSpacingScreen // to go from tile(x,y) to tile(x+1,y) add glm::ivec2(1,0) * tileSpacing
      )
      : tileSizeWorld{tileSizeWorld}
      , tileSizeScreen{tileSizeScreen}
      , tileSpacingScreen{tileSpacingScreen}
      , tile0{tileSizeScreen.x, tileSizeScreen.y}
    {
      using namespace raycasting;
      using namespace raycasting::shapes;

      // generate tile image

      const Sphere sphere{glm::vec3{0.f}, tileSizeWorld.x * 0.45f};
      const Intersectable &intersectable = sphere;

      glm::vec3 minLight{0.2f};
      std::vector<DirectionalLight> directionalLights{DirectionalLight{glm::normalize(forward), glm::vec3{1.f, 1.f, 1.f}}};

      camera.render(
        tile0.getUnsafeView(),
        intersectable,
        minLight,
        &directionalLights[0],
        directionalLights.size());
    }

    void render(
      const ViewOfCpuFrameBuffer &frameBuffer,
      glm::ivec2 screenCenterInWorld)
    {
      frameBuffer.clear(0xff000000, 0x7fff);

      glm::ivec2 frameBufferSize{frameBuffer.w, frameBuffer.h};
      glm::ivec2 destpos = frameBufferSize / 2 - tileSizeScreen / 2;

      for (int x = -3; x < 3; ++x)
      {
        drawWithDepth(
          frameBuffer,
          destpos.x + x * 21, destpos.y,
          tile0.getUnsafeView(),
          x * -5);
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

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, defaults::render::scaleQuality);

    setPlatformSpecificSdlHints();

    SdlWindow window{sdl, defaults::window::width, defaults::window::height};
    SdlRenderer renderer{window};
    FrameBuffers frameBuffers{renderer, defaults::render::scale};

    //----------------------------------------------------------------------------------------------------------------------
    // TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING TESTING
    //auto testRenderer =
    //  [](const ViewOfCpuFrameBuffer &imageWithDepth)
    //  {
    //    for (int y = 0; y < imageWithDepth.h; ++y)
    //      for (int x = 0; x < imageWithDepth.w; ++x)
    //      {
    //        uint32_t p = (0xFF000000) | ((x & y & 255) << 16) | ((x & ~y & 255) << 8) | (~x & ~y & 255);
    //        imageWithDepth.image[y * imageWithDepth.w + x] = p;
    //      }
    //  };

    // TEMPORARY: prepare test sprite / tile
    constexpr const int spriteWidth = 128, spriteHeight = 128;
    raycasting::OrthogonalCamera camera{.w = (float)spriteWidth * raycasting::right};

    // ANGLES
    // if the desired w:h ratio is 2:1 then angle above horizon should be 30 deg
    // formula:
    //   angle above horizon = 90 deg - atan(sqrt(ratio^2 - 1))
    // TABLE
    //   w h ang
    //   2 1 30
    //   5 3 36.8698976
    //   3 2 41.8103149
    //   4 3 48.5903779
    //   5 4 53.1301024

    constexpr auto angleInDegreesFromWidthToHeightRatio = [](int w, int h)
    {
      const float ratio = (float)w / h;
      return 90.f - glm::degrees(glm::atan(glm::sqrt(ratio * ratio - 1.f)));
    };

    struct Ratio {int w, h;};

    Ratio ratios[] = {{2, 1}, {5, 3}, {3, 2}, {4, 3}, {5, 4}};

    for (Ratio &ratio: ratios)
    {
      std::cout << "ratio: " << ratio.w << "/" << ratio.h << " angle: " << angleInDegreesFromWidthToHeightRatio(ratio.w, ratio.h) << std::endl;
    }

    constexpr float angleAboveHorizon = 30.f;
    //const float angleAboveHorizon = angleInDegreesFromWidthToHeightRatio(3,2);
    constexpr float angleAroundVertical = 45.f;

    const glm::mat4 rotAboveHorizon{glm::rotate(glm::mat4(1.f), glm::radians(angleAboveHorizon), raycasting::right)};
    const glm::mat4 rotAboveHorizonThenAroundVertical{glm::rotate(rotAboveHorizon, glm::radians(angleAroundVertical), raycasting::up)};

    auto mul = [](glm::vec3 v, glm::mat4 m)
    {
      return glm::vec3(glm::vec4(v, 1.f) * m);
    };

    //camera.position = mul(raycasting::backward * cameraDistance, rotAboveHorizonThenAroundVertical);
    camera.normal = glm::normalize(mul(raycasting::forward, rotAboveHorizonThenAroundVertical));
    camera.w = (float)spriteWidth * mul(raycasting::right, rotAboveHorizonThenAroundVertical);
    camera.h = (float)spriteHeight * mul(raycasting::up, rotAboveHorizonThenAroundVertical);

    // TODO: delete
    // camera math debugging
    {
      {
        float w_to_x = glm::dot(raycasting::right, camera.w) / glm::length(camera.w);
        float h_to_y = glm::dot(raycasting::forward, camera.h) / glm::length(camera.h);

        std::cout << "w_to_x: " << w_to_x << ", h_to_y: " << h_to_y << std::endl;
      }
    }

    //testing::SpriteRenderer spriteRenderer{camera, cameraDistance, 128, 128};
    glm::imat2x2 tileSpacingScreen;
    testing::TileRenderer tileRenderer{
      camera,
      glm::ivec2{64, 64},
      glm::ivec2{128, 128},
      tileSpacingScreen};

    // render loop
    for (bool quit = false; !quit;)
    {
      // handle SDL events
      for (SDL_Event e; 0 != SDL_PollEvent(&e);)
      {
        if (e.type == SDL_QUIT)
          quit = true;
      }

      glm::ivec2 screenCenterInWorld{0,0};

      auto tstart = clock::now();
      //frameBuffers.renderWith(testRenderer);
      //frameBuffers.renderWith(renderSphere);
      //frameBuffers.renderWith([&](const ViewOfCpuFrameBuffer &frameBuffer) {spriteRenderer.render(frameBuffer);});
      frameBuffers.renderWith([&](const ViewOfCpuFrameBuffer &frameBuffer) {tileRenderer.render(frameBuffer,screenCenterInWorld);});
      auto elapsedmillis = (1.0 / 1000.0) * std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - tstart).count();
      //double elapsedmillis = (double)elapsedmicros / 1000.0;

      SDL_SetWindowTitle(window.window, toString(defaults::window::title, " render millis: ", elapsedmillis).c_str());

      frameBuffers.present();
    }

    //    SDL_Delay(3000);
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
