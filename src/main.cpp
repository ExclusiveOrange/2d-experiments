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
  
  struct ViewOfCpuFrameBuffer
  {
    uint32_t *image;
    int32_t *depth;
    int w, h;
    
    void clear(uint32_t argbClearValue = 0xff000000, int32_t depthClearValue = 0) const
    {
      std::fill_n(image, w * h, argbClearValue);
      std::fill_n(depth, w * h, depthClearValue);
    }
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
  
  void
  drawWithoutDepth(
    ViewOfCpuFrameBuffer dest, int destx, int desty,
    ViewOfCpuImageWithDepth src)
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
          uint32_t sargb = 0xff000000 | (sdrgb & 0xffffff);
          dest.image[dindex] = sargb;
        }
      }
    }
  }
  
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
  
  // note: use std::optional to contain this if you want a replaceable object, then use std::optional.emplace(...)
  struct CpuImageWithDepth
  {
    CpuImageWithDepth(int w, int h)
      : drgb{std::make_unique<uint32_t[]>(w * h)}
      , w{w}, h{h} {}
    
    ViewOfCpuImageWithDepth
    getUnsafeView() const {return {.drgb = drgb.get(), .w = w, .h = h};}
  
  private:
    const std::unique_ptr<uint32_t[]> drgb;
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
  
  //======================================================================================================================
  // TEMPORARY maybe, for rendering tiles / sprites for testing since I can't find a good ready-made application
  //   that lets me create a sprite with alpha channel as depth; maybe it can be done with Blender...
  
  struct Ray
  {
    glm::vec3 origin, unitDirection;
  };
  
  struct Intersection
  {
    glm::vec3 position, normal;
    glm::vec3 diffuse;
    float distance;
  };
  
  
  //======================================================================================================================
  // TODO: move to separate file
  
  
  namespace raycasting
  {
    constexpr const glm::vec3 right{1.f, 0.f, 0.f};
    constexpr const glm::vec3 left = -right;
    
    constexpr const glm::vec3 forward{0.f, 1.f, 0.f};
    constexpr const glm::vec3 backward = -forward;
    
    constexpr const glm::vec3 up{0.f, 0.f, 1.f};
    constexpr const glm::vec3 down = -up;
    
    // NOTE: shadows
    // Probably want to avoid shadows on pre-rendered objects for now in order to avoid
    // contradictions with varying lighting environments during run-time.
    
    struct Intersectable
    {
      virtual ~Intersectable() {}
      
      virtual std::optional<Intersection> intersect(const Ray &ray) const = 0;
    };
    
    struct DirectionalLight
    {
      DirectionalLight(glm::vec3 direction, glm::vec3 intensity)
        : ndirection{-direction}, intensity{intensity} {}
      
      glm::vec3 calculate(glm::vec3 position, glm::vec3 normal) const
      {
        return intensity * glm::dot(normal, ndirection);
      }
    
    private:
      const glm::vec3 ndirection, intensity;
    };
    
    struct OrthogonalCamera
    {
      glm::vec3 position, normal;
      glm::vec3 w, h;
    };
    
    void
    renderOrthogonal(
      const ViewOfCpuImageWithDepth &destImage,
      const OrthogonalCamera &camera,
      const Intersectable &intersectable,
      const DirectionalLight *directionalLights,
      int numDirectionalLights,
      float minDepth, // becomes uint8_t 0 (anything less is clamped at 0)
      float maxDepth) // becomes uint8_t 254 (anything greater is clamped at 255, the special transparency value)
    {
      const glm::vec3 horStep = camera.w / (float)destImage.w;
      const glm::vec3 verStep = camera.h / (float)destImage.h;
      const float rDepthRange = 254.f / (maxDepth - minDepth);
      
      Ray ray{.unitDirection = camera.normal};
      
      for (int y = 0; y < destImage.h; ++y)
      {
        glm::vec3 yOffset = (destImage.h / -2.f + y + 0.5f) * verStep;
        
        for (int x = 0; x < destImage.w; ++x)
        {
          glm::vec3 xOffset = (destImage.w / -2.f + x + 0.5f) * horStep;
          ray.origin = camera.position + yOffset + xOffset;
          
          uint32_t drgb;
          
          if (std::optional<Intersection> i = intersectable.intersect(ray))
          {
            glm::vec3 lightSum{};
            
            for (int il = 0; il < numDirectionalLights; ++il)
              lightSum += directionalLights[il].calculate(i->position, i->normal);
            
            glm::vec3 color = 255.f * glm::clamp(lightSum, 0.f, 1.f);
            uint8_t depth = (uint8_t)glm::clamp((i->distance - minDepth) * rDepthRange, 0.f, 255.f);
            
            drgb = (depth << 24) | (uint32_t(color.x) << 16) | (uint32_t(color.y) << 8) | uint32_t(color.z);
          }
          else
            drgb = 0xff000000;
          
          int dindex = y * destImage.w + x;
          destImage.drgb[dindex] = drgb;
        }
      }
    }
  }
  
  namespace raycasting::shapes
  {
    struct Sphere : Intersectable
    {
      Sphere(glm::vec3 center, float radius)
        : center{center}, sqradius{radius * radius} {}
      
      std::optional<Intersection>
      intersect(const Ray &ray) const override
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
    
    //    auto renderSphere =
    //      [](const ViewOfCpuFrameBuffer &imageWithDepth)
    //      {
    //        constexpr const float rayOriginDistance = 50.f;
    //        constexpr const float viewAngle = glm::radians(30.f);
    //        constexpr const float scale = 6.f;
    //
    //        // right: +x
    //        // forward: +y
    //        // up: +z
    //
    //        constexpr const glm::vec3 right{1.f, 0.f, 0.f};
    //        constexpr const glm::vec3 left = -right;
    //
    //        constexpr const glm::vec3 forward{0.f, 1.f, 0.f};
    //        constexpr const glm::vec3 backward = -forward;
    //
    //        constexpr const glm::vec3 up{0.f, 0.f, 1.f};
    //        constexpr const glm::vec3 down = -up;
    //
    //        const Sphere sphere{glm::vec3{0.f}, 40.f};
    //        constexpr const glm::vec3 lightPosition{right * 100.f /*+ up * 100.f*/ + backward * 100.f};
    //        constexpr const glm::vec3 lightIntensity{1.f};
    //
    //        // distance is somewhat arbitrary but should be far enough back from object that it doesn't clip
    //        // backward = k * cos(viewAngle)
    //        // up = k * sin(viewAngle)
    //        glm::vec3 centerRayOrigin{backward * rayOriginDistance * glm::cos(viewAngle) + up * rayOriginDistance * glm::sin(viewAngle)};
    //        glm::vec3 unitRayDirection{glm::normalize(forward * glm::cos(viewAngle) + down * glm::sin(viewAngle))};
    //        glm::vec3 unitRelativeUp{glm::normalize(glm::cross(unitRayDirection, right))};
    //        glm::vec3 xStep = right / scale;
    //        glm::vec3 yStep = unitRelativeUp / scale;
    //
    //        Ray ray{.unitDirection= unitRayDirection};
    //
    //        // TODO: might need to adjust position by half a pixel one way or the other
    //        for (int y = 0; y < imageWithDepth.h; ++y)
    //        {
    //          glm::vec3 yOffset = (imageWithDepth.h / -2.f + y + 0.5f) * yStep;
    //
    //          for (int x = 0; x < imageWithDepth.w; ++x)
    //          {
    //            glm::vec3 xOffset = (imageWithDepth.w / -2.f + x + 0.5f) * xStep;
    //            ray.origin = centerRayOrigin + yOffset + xOffset;
    //
    //            uint8_t distance = 255;
    //            glm::vec3 color{};
    //
    //            if (std::optional<Intersection> i = sphere.nearestIntersection(ray))
    //            {
    //              glm::vec3 directionToLight{glm::normalize(lightPosition - i->position)};
    //              color = lightIntensity * glm::dot(i->normal, directionToLight);
    //              color = glm::clamp(color, glm::vec3{0.f}, glm::vec3{1.f});
    //              color *= 255.f;
    //              distance = (uint8_t)glm::clamp(i->distance, 0.f, 255.f);
    //            }
    //
    //            uint32_t pixel = 0xff000000 | (uint32_t(color.x) << 16) | (uint32_t(color.y) << 8) | uint32_t(color.z);
    //
    //            int dindex = y * imageWithDepth.w + x;
    //            imageWithDepth.image[dindex] = pixel;
    //            imageWithDepth.depth[dindex] = distance;
    //          }
    //        }
    //      };
    
    // TEMPORARY: prepare test sprite / tile
    constexpr const int spriteWidth = 128, spriteHeight = 128;
    CpuImageWithDepth spriteSphere{spriteWidth, spriteHeight};
    
    {
      using namespace raycasting;
      using namespace raycasting::shapes;
      
      constexpr float cameraDistanceFromOrigin = 200.f;
      constexpr float sphereRadius = 50.f;
      constexpr float depthRange = 128.f;
      
      OrthogonalCamera camera{
        .position = backward * cameraDistanceFromOrigin,
        .normal = forward,
        .w = (float)spriteWidth * right, .h = (float)spriteHeight * up};
      
      constexpr float minDepth = cameraDistanceFromOrigin - depthRange / 2;
      constexpr float maxDepth = cameraDistanceFromOrigin + depthRange / 2;
      
      Sphere sphere{glm::vec3{0.f}, sphereRadius};
      
      std::vector<DirectionalLight> directionalLights{
        DirectionalLight{forward, glm::vec3{1.f, 1.f, 1.f}}};
      
      renderOrthogonal(
        spriteSphere.getUnsafeView(),
        camera,
        sphere,
        &directionalLights[0],
        directionalLights.size(),
        minDepth, maxDepth);
    }
    
    // TEMPORARY: function to render scene with a sprite
    auto renderSprite =
      [&](const ViewOfCpuFrameBuffer &imageWithDepth)
      {
        imageWithDepth.clear(0xff000000, 0x7fffffff);
        
        const ViewOfCpuImageWithDepth sprite = spriteSphere.getUnsafeView();
        
        //drawWithoutDepth(
        //  imageWithDepth,
        //  imageWithDepth.w / 2 - sprite.w / 2, imageWithDepth.h / 2 - sprite.h / 2,
        //  sprite);
        
        glm::ivec2 destCenter{imageWithDepth.w / 2, imageWithDepth.h / 2};
        glm::ivec2 spriteSize{sprite.w, sprite.h};
        
        for (int i = -10; i < 10; ++i)
        {
          glm::ivec2 destPos = destCenter - spriteSize / 2 + i * glm::ivec2{spriteSize.x / 4, 0};
  
          drawWithDepth(
            imageWithDepth,
            destPos.x, destPos.y,
            sprite,
            128);
          
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
