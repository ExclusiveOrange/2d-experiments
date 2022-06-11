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
    
    // TEMPORARY: prepare test sprite / tile
    constexpr const int spriteWidth = 128, spriteHeight = 128;
    CpuImageWithDepth spriteSphere{spriteWidth, spriteHeight};
    
    {
      using namespace raycasting;
      using namespace raycasting::shapes;
      
      constexpr float cameraDistanceFromOrigin = 200.f;
      constexpr float sphereRadius = 62.f;
      constexpr float depthRange = 128.f;
      
      OrthogonalCamera camera{
        .position = backward * cameraDistanceFromOrigin,
        .normal = forward,
        .w = (float)spriteWidth * right, .h = (float)spriteHeight * up};
      
      constexpr float minDepth = cameraDistanceFromOrigin - depthRange / 2;
      constexpr float maxDepth = cameraDistanceFromOrigin + depthRange / 2;
      
      Sphere sphere{glm::vec3{0.f}, sphereRadius};
      
      glm::vec3 minLight{0.2f, 0.15f, 0.1f};
      
      std::vector<DirectionalLight> directionalLights{
        DirectionalLight{glm::normalize(2.f * forward + down), glm::vec3{1.f, 1.f, 1.f}}};
      
      camera.render(
        spriteSphere.getUnsafeView(),
        sphere,
        minLight,
        &directionalLights[0],
        directionalLights.size(),
        minDepth, maxDepth);
    }

    // TEMPORARY: function to render scene with a sprite
    auto renderSprite =
      [&](const ViewOfCpuFrameBuffer &imageWithDepth)
      {
        imageWithDepth.clear(0xff000000, 0x7fff);

        const ViewOfCpuImageWithDepth sprite = spriteSphere.getUnsafeView();
        const glm::ivec2 destCenter{imageWithDepth.w / 2, imageWithDepth.h / 2};
        const glm::ivec2 spriteSize{sprite.w, sprite.h};
        const glm::ivec2 spacing{spriteSize.x / 2, spriteSize.y / 4};
        const glm::ivec2 steps = 1 + glm::ivec2{imageWithDepth.w, imageWithDepth.h} / (2 * spacing);

        for (glm::ivec2 pos{-steps}; pos.y <= steps.y; ++pos.y)
          for (pos.x = -steps.x; pos.x <= steps.x; ++pos.x)
          {
            glm::ivec2 destPos = destCenter - spriteSize / 2 + pos * spacing;
            int depthBias = 2 * destPos.y;
            destPos.x += (pos.y & 1) * spacing.x / 2;
            drawWithDepth(
              imageWithDepth,
              destPos.x, destPos.y,
              sprite,
              depthBias);
          }
      };
    
    // render loop
    for (bool quit = false; !quit;)
    {
      // handle SDL events
      for (SDL_Event e; 0 != SDL_PollEvent(&e);)
      {
        if (e.type == SDL_QUIT)
          quit = true;
      }
      
      auto tstart = clock::now();
      //frameBuffers.renderWith(testRenderer);
      //frameBuffers.renderWith(renderSphere);
      frameBuffers.renderWith(renderSprite);
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
