#include <iostream>
#include <filesystem>
#include <functional>
#include <future>
#include <optional>
#include <utility>

#include <SDL.h>
#include <SDL_image.h>
#include <NoCopyNoMove.hpp>

#include "Destroyer.hpp"
#include "function_traits.hpp"
#include "NoCopy.hpp"
#include "toString.hpp"

namespace
{
//  using errmsg = std::optional<std::string>;

  template<class...Args>
  auto exception(Args &&...args)
  {
    return std::runtime_error(toString(std::forward<Args>(args)...));
  }

  namespace paths
  {
    const std::filesystem::path assets = "assets";
    const std::filesystem::path images = assets / "images";
  }

//  errmsg
//  withImages(Function<errmsg(const Images &)> auto &&f)
//  {
//    errmsg errormsg{};
//    Images images{};
//    bool allImagesLoaded = !(errormsg = loadImage(Images::test1_file, &images.test1));
//
//    if (allImagesLoaded)
//      return f(images);
//
//    return errormsg;
//  }
//
//  errmsg
//  loadSdlThen(Function<errmsg()> auto &&f)
//  {
//    if (SDL_Init(SDL_INIT_VIDEO) < 0)
//      return toString("SDL_Init failed! SDL_Error: ", SDL_GetError());
//
//    errmsg errormsg = f();
//    SDL_Quit();
//    return errormsg;
//  }
//
//  errmsg
//  createSdlWindowThen(int width, int height, Function<errmsg(SDL_Window *)> auto &&f)
//  {
//    SDL_Window *sdlWindow =
//      SDL_CreateWindow(
//        "SDL Tutorial",
//        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
//        width, height,
//        SDL_WINDOW_SHOWN);
//
//    if (sdlWindow == nullptr)
//      return toString("SDL_CreateWindow failed! SDL_Error: ", SDL_GetError());
//
//    errmsg errormsg = f(sdlWindow);
//    SDL_DestroyWindow(sdlWindow);
//    return errormsg;
//  }
//
//  errmsg
//  runWithWindowAndImages(SDL_Window *sdlWindow, const Images &images)
//  {
//    auto surface = SDL_GetWindowSurface(sdlWindow);
//
//    SDL_BlitSurface(images.test1, nullptr, surface, nullptr);
//    SDL_UpdateWindowSurface(sdlWindow);
//
//    // DELETE
//    SDL_Delay(2000);
//
//    return {};
//  }
//
//  errmsg
//  showSplashScreenThen(SDL_Window *sdlWindow)
//  {
//    SDL_Surface *sdlSurface = SDL_GetWindowSurface(sdlWindow);
//
//    // load screen I guess
//    SDL_FillRect(sdlSurface, nullptr, SDL_MapRGB(sdlSurface->format, 255, 255, 255));
//    SDL_UpdateWindowSurface(sdlWindow);
//
//    return withImages([=](const Images &images) -> errmsg {return runWithWindowAndImages(sdlWindow, images);});
//  }

//  struct _LoadSdl
//  {
//    [[nodiscard]] _LoadSdl()
//    {
//      if (SDL_Init(SDL_INIT_VIDEO) < 0)
//        throw std::runtime_error(toString("SDL_Init failed: ", SDL_GetError()));
//    }
//
//    ~_LoadSdl()
//    {
//      SDL_Quit();
//    }
//
//    template<class TypeToCreate, class... CreationArgs>
//    TypeToCreate then(CreationArgs &&...args)
//    {
//      return TypeToCreate{std::forward<CreationArgs>(args)...};
//    }
//  };

  struct HasSdlWindow
  {
    HasSdlWindow(SDL_Window *sdlWindow)
      : sdlWindow{sdlWindow} {}

  protected:
    SDL_Window *sdlWindow{};
  };

//  struct CreateSdlWindow : public HasSdlWindow
//  {
//    CreateSdlWindow(int width, int height)
//      : HasSdlWindow{
//      SDL_CreateWindow(
//        "SDL Tutorial",
//        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
//        width, height,
//        SDL_WINDOW_SHOWN)}
//    {
//      if (sdlWindow == nullptr)
//        throw std::runtime_error(toString("SDL_CreateWindow failed: ", SDL_GetError()));
//    }
//
//    ~CreateSdlWindow()
//    {
//      SDL_DestroyWindow(sdlWindow);
//    }
//
//    template<class TypeToCreate, class... CreationArgs>
//    TypeToCreate thenWithWindow(CreationArgs &&...args)
//    {
//      return TypeToCreate{sdlWindow, std::forward<CreationArgs>(args)...};
//    }
//  };

//  struct ShowSplashScreen : HasSdlWindow
//  {
//    ShowSplashScreen(SDL_Window *sdlWindow)
//      : HasSdlWindow{sdlWindow}
//    {
//      SDL_Surface *sdlSurface = SDL_GetWindowSurface(sdlWindow);
//      SDL_FillRect(sdlSurface, nullptr, SDL_MapRGB(sdlSurface->format, 255, 255, 255));
//      SDL_UpdateWindowSurface(sdlWindow);
//    }
//
//    template<class TypeToCreate, class... CreationArgs>
//    TypeToCreate thenWithWindow(CreationArgs &&...args)
//    {
//      return TypeToCreate{sdlWindow, std::forward<CreationArgs>(args)...};
//    }
//  };

//  struct HasImages
//  {
//    HasImages(Images *images) : images{images} {}
//
//  protected:
//    Images *images;
//  };
//
//  struct LoadImages : HasSdlWindow, HasImages
//  {
//    LoadImages(SDL_Window *sdlWindow)
//      : HasSdlWindow{sdlWindow}
//      , HasImages{new Images}
//    {
//      try
//      {
//        loadImage(Images::test1_file, &images->test1);
//      }
//      catch (const std::exception &e)
//      {
//        delete images;
//        images = nullptr;
//        throw;
//      }
//    }
//
//    ~LoadImages()
//    {
//      delete images;
//    }
//
//    template<class TypeToCreate, class... CreationArgs>
//    TypeToCreate thenWithWindowAndImages(CreationArgs &&...args)
//    {
//      return TypeToCreate{sdlWindow, images, std::forward<CreationArgs>(args)...};
//    }
//  };
//
//  struct ShowTestImage
//  {
//    ShowTestImage(SDL_Window *sdlWindow, Images *images)
//    {
//      auto surface = SDL_GetWindowSurface(sdlWindow);
//
//      SDL_BlitSurface(images->test1, nullptr, surface, nullptr);
//      SDL_UpdateWindowSurface(sdlWindow);
//
//      SDL_Delay(2000);
//    }
//  };

  struct Sdl
  {
    [[nodiscard]] Sdl()
    {
      if (SDL_Init(SDL_INIT_VIDEO) < 0)
        throw std::runtime_error(toString("SDL_Init failed: ", SDL_GetError()));
    }

    ~Sdl() {SDL_Quit();}

  private:
    Sdl(const Sdl &) = delete;
    Sdl(Sdl &&) = delete;
    Sdl &operator=(const Sdl &) = delete;
    Sdl &operator=(Sdl &&) = delete;
  };

  struct SdlWindow
  {
    SdlWindow(int width, int height)
      : window{
      SDL_CreateWindow(
        "SDL Tutorial",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_SHOWN)}
    {
      if (window == nullptr)
        throw exception("SDL_CreateWindow failed: ", SDL_GetError());
    }

    SDL_Window *get() {return window;}
    ~SdlWindow() {SDL_DestroyWindow(window);}

  private:
    SDL_Window *window{};
  };

  void ShowSplashScreen(SDL_Window *window)
  {
    SDL_Surface *sdlSurface = SDL_GetWindowSurface(window);
    SDL_FillRect(sdlSurface, nullptr, SDL_MapRGB(sdlSurface->format, 255, 255, 255));
    SDL_UpdateWindowSurface(window);
  }

  struct Images
  {
    Images() = default;
    Images(Images &&other) {*this = std::move(other);}
    Images &operator=(Images &&other) {return (*this = other, other = {}, *this);}

    Images(const std::filesystem::path &imagesPath)
    {
      auto tryLoadImage = [&imagesPath](const char *filename)
      {
        std::filesystem::path path{imagesPath / filename};

        SDL_Surface *maybeSurface = IMG_Load(path.string().c_str());

        if (!maybeSurface)
          throw std::runtime_error(toString("IMG_Load failed: ", SDL_GetError()));

        return maybeSurface;
      };

      test1 = tryLoadImage(test1File);
    }

    static constexpr const char *test1File = "2-1 terrain tile 1.png";
    const SDL_Surface *getTest1() {return test1;}

    ~Images()
    {
      SDL_FreeSurface(test1);
    }

  private:
    Images(const Images &) = delete;
    Images &operator=(const Images &) = default;

    SDL_Surface *test1{};
  };
}

int main(int argc, char *argv[])
{
  constexpr int width = 1920, height = 1080;

  try
  {
    Sdl sdl;
    SdlWindow sdlWindow{width, height};
    ShowSplashScreen(sdlWindow.get());

    // load images in another thread
    std::unique_ptr<Images> images;

    {
      std::cout << "loading images..." << std::flush;
      std::future<std::unique_ptr<Images>> futureImages =
        std::async(std::launch::async, [] {return std::make_unique<Images>(paths::images);});

      while (futureImages.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        std::cout << "." << std::flush;// TODO: show loading animation on sdlWindow

      std::cout << "done." << std::endl;

      images = futureImages.get();
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "caught exception in main: " << e.what() << std::endl;
  }

  // Note: this part is less readable but it doesn't need exceptions and data can skip functions by being captured
  // in lambdas.

//  errmsg errormsg = loadSdlThen([=] {return createSdlWindowThen(width, height, showSplashScreenThen);});
//
//  if (errormsg)
//    std::cerr << "error: " << *errormsg << std::endl;

  return 0;
}
