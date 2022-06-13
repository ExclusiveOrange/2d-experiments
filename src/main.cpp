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
#include <limits>
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
#include "MovementVectors.hpp"
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
    constexpr const float scale = 1.f;
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
  
  template< class...Args >
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
  class TileRenderer : NoCopyNoMove
  {
    static constexpr int tileIntervalWorld = 100;
    static constexpr int tileMarginWorld = 1;

    const glm::mat3 screenToWorld;
    const glm::mat3 worldToScreen;

    std::optional<CpuImageWithDepth> tile0;

    static glm::ivec2 calculateTileScreenSize(glm::mat3 worldToScreen)
    {
      glm::vec3 tileMaxWorld{tileIntervalWorld * 0.5f + tileMarginWorld};
      glm::vec3 tileMinWorld{-tileMaxWorld};

      // only need to measure one extreme since the other will be the negation of that
      glm::vec3 screenMax{std::numeric_limits<float>::min()};

      for (int i = 0; i < 8; ++i)
      {
        glm::vec3 world{i & 1 ? tileMaxWorld.x : tileMinWorld.x,
                        i & 2 ? tileMaxWorld.y : tileMinWorld.y,
                        i & 4 ? tileMaxWorld.z : tileMinWorld.z};
        glm::vec3 screen = world * worldToScreen;
        screenMax = glm::max(screen, screenMax);
      }

      return 2 * glm::ivec2(glm::ceil(glm::vec2(screenMax)));
    }

  public:
    TileRenderer(
      const raycasting::OrthogonalCamera &camera,
      glm::mat3 screenToWorld,
      glm::mat3 worldToScreen)
      : screenToWorld{screenToWorld}
      , worldToScreen{worldToScreen}
      , tile0{}
    {
      using namespace raycasting;
      using namespace raycasting::shapes;

      // generate tile image
      glm::ivec2 tileImageSize{calculateTileScreenSize(worldToScreen)};
      tile0.emplace(tileImageSize.x, tileImageSize.y);

      std::cout << toString("TileRenderer: tileIntervalWorld(", tileIntervalWorld, "), tileMarginWorld(", tileMarginWorld, "), tileImageSize(", tileImageSize.x, " x ", tileImageSize.y, ")\n");

      //const Sphere sphere{glm::vec3{0.f}, tileIntervalWorld * 0.45f};
      const float halfIntervalPlusMargin = tileIntervalWorld * 0.45f + tileMarginWorld;
      const Quad quad{glm::vec3{0.f}, halfIntervalPlusMargin * forward, halfIntervalPlusMargin * right};
      //const Intersectable &intersectable = quad;

      glm::vec3 minLight{0.2f};
      std::vector<DirectionalLight> directionalLights{DirectionalLight{glm::normalize(forward + 2.f * down), glm::vec3{1.f, 1.f, 1.f}}};

      auto intersect = [&](const Ray &ray)
      {
        return quad.intersect(ray);
      };

      camera.render(
        tile0->getUnsafeView(),
        intersect,
        minLight,
        &directionalLights[0],
        directionalLights.size());
    }

    void render(const ViewOfCpuFrameBuffer &frameBuffer, glm::ivec3 screenCenterInWorld)
    {
      frameBuffer.clear(0xff000000, 0x7fff);

      int interval = tileIntervalWorld;

      // TODO: figure out how to enumerate all visible tiles and no non-visible tiles,
      // given that the camera angle hasn't been set in stone yet so the formula should be generalized for now
      // Probably could use flood-fill where a tile is a candidate if it overlaps the screen.

      for (int y = -10; y < 10; ++y)
        for (int x = -10; x < 10; ++x)
        {

          glm::vec3 worldCoords{(float)x * interval, (float)y * interval, 0.f};
          glm::ivec3 screenCoords{worldCoords * worldToScreen};

          ViewOfCpuImageWithDepth tileView = tile0->getUnsafeView();

          drawWithDepth(
            frameBuffer,
            frameBuffer.w / 2 + screenCoords.x - tileView.w / 2,
            frameBuffer.h / 2 + screenCoords.y - tileView.h / 2,
            tileView,
            screenCoords.z);
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
    raycasting::OrthogonalCamera camera{};
    
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
    
    // ratio experiments
    //{
    //  struct Ratio {int w, h;};
    //  Ratio ratios[] = {{2, 1}, {5, 3}, {3, 2}, {4, 3}, {5, 4}};
    //  for (Ratio &ratio: ratios)
    //    std::cout << "ratio: " << ratio.w << "/" << ratio.h << " angle: " << angleInDegreesFromWidthToHeightRatio(ratio.w, ratio.h) << std::endl;
    //}
    
    //const float angleAboveHorizon = angleInDegreesFromWidthToHeightRatio(3,2);
    
    constexpr float angleAboveHorizon = 30.f;
    constexpr float angleAroundVertical = 30.f;
    
    glm::mat3x3{glm::mat4x4{}};
    
    glm::mat3 cameraRotation = glm::mat3(glm::rotate(glm::rotate(glm::mat4(1.f), glm::radians(angleAboveHorizon), raycasting::right), glm::radians(angleAroundVertical), raycasting::up));
    camera.normal = raycasting::forward * cameraRotation; // at angle 0 the camera forward (+z) is world forward
    camera.xstep = raycasting::right * cameraRotation; // at angle 0 the camera +x is world right
    camera.ystep = raycasting::up * cameraRotation; // at angle 0 the camera +y is world up
    
    // TODO: delete
    // camera math debugging
    {
      std::cout
        << "camera.normal: " << camera.normal << "\n"
        << "camera.xstep: " << camera.xstep << "\n"
        << "camera.ystep: " << camera.ystep << "\n";
    }
    
    // because of my choice for world and camera coordinates, it is necessary to swap y and z in the worldToScreen transform here to get the expected results elsewhere
    glm::mat3 worldToScreen = glm::inverse(cameraRotation);
    std::swap(worldToScreen[1], worldToScreen[2]);
    glm::mat3 screenToWorld = glm::inverse(worldToScreen);

    // TODO: delete
    // testing movement vectors
    {
      MovementVectors movementVectors{screenToWorld};

      std::cout << "movement vectors:\n" << movementVectors << std::endl;
    }
    
    testing::TileRenderer tileRenderer{camera, screenToWorld, worldToScreen};

    // render loop
    for (bool quit = false; !quit;)
    {
      // handle SDL events
      for (SDL_Event e; 0 != SDL_PollEvent(&e);)
      {
        if (e.type == SDL_QUIT)
          quit = true;
      }
      
      glm::vec3 screenCenterInWorld{0.f};
      
      auto tstart = clock::now();
      frameBuffers.renderWith([&](const ViewOfCpuFrameBuffer &frameBuffer) {tileRenderer.render(frameBuffer, screenCenterInWorld);});
      auto elapsedmillis = (1.0 / 1000.0) * std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - tstart).count();
      
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
