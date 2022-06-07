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

  namespace paths
  {

    const std::filesystem::path assets = "assets";
    const std::filesystem::path images = assets / "images";
  }

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

  void loadImage(const char *filename, SDL_Surface **dest)
  {
    std::filesystem::path path = paths::images / filename;
    SDL_Surface *maybeSurface = IMG_Load(path.string().c_str());

    if (!maybeSurface)
      throw std::runtime_error(toString("IMG_Load failed: ", SDL_GetError()));

    *dest = maybeSurface;
  }
//
//  errmsg loadImage(const char *filename, SDL_Surface **dest)
//  {
//    std::filesystem::path path = paths::images / filename;
//
//    if (SDL_Surface *maybeSurface = IMG_Load(path.string().c_str()))
//      return (*dest = maybeSurface, std::nullopt);
//
//    return toString("IMG_Load failed! SDL_Error: ", SDL_GetError());
//  }
//
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

  struct LoadSdl
  {
    LoadSdl()
    {
      if (SDL_Init(SDL_INIT_VIDEO) < 0)
        throw std::runtime_error(toString("SDL_Init failed: ", SDL_GetError()));
    }

    ~LoadSdl()
    {
      SDL_Quit();
    }

    template<class TypeToCreate, class... CreationArgs>
    TypeToCreate then(CreationArgs &&...args)
    {
      return TypeToCreate{std::forward<CreationArgs>(args)...};
    }
  };

  struct HasSdlWindow
  {
    HasSdlWindow(SDL_Window *sdlWindow)
      : sdlWindow{sdlWindow} {}

  protected:
    SDL_Window *sdlWindow{};
  };

  struct CreateSdlWindow : public HasSdlWindow
  {
    CreateSdlWindow(int width, int height)
      : HasSdlWindow{
      SDL_CreateWindow(
        "SDL Tutorial",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_SHOWN)}
    {
      if (sdlWindow == nullptr)
        throw std::runtime_error(toString("SDL_CreateWindow failed: ", SDL_GetError()));
    }

    ~CreateSdlWindow()
    {
      SDL_DestroyWindow(sdlWindow);
    }

    template<class TypeToCreate, class... CreationArgs>
    TypeToCreate thenWithWindow(CreationArgs &&...args)
    {
      return TypeToCreate{sdlWindow, std::forward<CreationArgs>(args)...};
    }
  };

  struct ShowSplashScreen : HasSdlWindow
  {
    ShowSplashScreen(SDL_Window *sdlWindow)
      : HasSdlWindow{sdlWindow}
    {
      SDL_Surface *sdlSurface = SDL_GetWindowSurface(sdlWindow);
      SDL_FillRect(sdlSurface, nullptr, SDL_MapRGB(sdlSurface->format, 255, 255, 255));
      SDL_UpdateWindowSurface(sdlWindow);
    }

    template<class TypeToCreate, class... CreationArgs>
    TypeToCreate thenWithWindow(CreationArgs &&...args)
    {
      return TypeToCreate{sdlWindow, std::forward<CreationArgs>(args)...};
    }
  };

  struct HasImages
  {
    HasImages(Images *images) : images{images} {}

  protected:
    Images *images;
  };

  struct LoadImages : HasSdlWindow, HasImages
  {
    LoadImages(SDL_Window *sdlWindow)
      : HasSdlWindow{sdlWindow}
      , HasImages{new Images}
    {
      try
      {
        loadImage(Images::test1_file, &images->test1);
      }
      catch (const std::exception &e)
      {
        delete images;
        images = nullptr;
        throw;
      }
    }

    ~LoadImages()
    {
      delete images;
    }

    template<class TypeToCreate, class... CreationArgs>
    TypeToCreate thenWithWindowAndImages(CreationArgs &&...args)
    {
      return TypeToCreate{sdlWindow, images, std::forward<CreationArgs>(args)...};
    }
  };

  struct ShowTestImage
  {
    ShowTestImage(SDL_Window *sdlWindow, Images *images)
    {
      auto surface = SDL_GetWindowSurface(sdlWindow);

      SDL_BlitSurface(images->test1, nullptr, surface, nullptr);
      SDL_UpdateWindowSurface(sdlWindow);

      SDL_Delay(2000);
    }
  };
}

int main(int argc, char *argv[])
{
  constexpr int width = 1920, height = 1080;

  try
  {
    LoadSdl()
      .then<CreateSdlWindow>(width, height)
      .thenWithWindow<ShowSplashScreen>()
      .thenWithWindow<LoadImages>()
      .thenWithWindowAndImages<ShowTestImage>();
  }
  catch (const std::exception &e)
  {
    std::cerr << "caught exception in main: " << e.what() << std::endl;
  }
//  errmsg errormsg = loadSdlThen([=] {return createSdlWindowThen(width, height, showSplashScreenThen);});
//
//  if (errormsg)
//    std::cerr << "error: " << *errormsg << std::endl;

  return 0;
}
