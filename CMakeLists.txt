cmake_minimum_required(VERSION 3.20)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(2d-experiments CXX)

# NOTICE: on Windows in CLion I was using vcpkg to manage my libraries,
# and my CLion CMake settings for this project look like this:
#
# File / Settings / Build, Execution, Deployment / CMake / (Profile) / CMake options:
#    -GNinja -DCMAKE_TOOLCHAIN_FILE=c:/vcpkg/scripts/buildsystems/vcpkg.cmake
#
# NOTICE: if on Windows using vcpkg and CMake suddenly can't find anything,
# consider reinstalling vcpkg (delete vcpkg directory, follow instructions online to re-install).
# No idea why, but mine stopped working and reinstalling it was the only way to get it working again.
# This also means using vcpkg to reinstall all dependencies.
#
# NOTICE: strongly recommend using clang-cl.exe instead of cl.exe on Windows.
# At least MSVC 2022 cl.exe has problems compiling constraints & concepts and reports compile errors for valid code.
# MSVC has always had problems with c++ templates and new or uncommon code patterns and you can save a lot of headache
# by using clang or g++ instead of MSVC's own compiler.

########################################################################################################################
# CLion build notes
#
#-----------------------------------------------------------------------------------------------------------------------
# Set 64-bit build output:
#
# File / Settings / Build, Execution, Deployment / Toolchains
#    Visual Studio:
#        Toolset:
#            Architecture: amd64
#        CMake:
#            C Compiler: <some path>/clang-cl.exe
#            C++ Compiler: <some path>/clang-cl.exe
#
#-----------------------------------------------------------------------------------------------------------------------
# Configure CMake to find vcpkg libraries:
#
# File / Settings / Build, Execution, Deployment / CMake / (Profile) / CMake options:
#    -GNinja -DCMAKE_TOOLCHAIN_FILE=c:/vcpkg/scripts/buildsystems/vcpkg.cmake
#
# NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE
# If on Windows using vcpkg and CMake suddenly can't find anything, then consider reinstalling vcpkg.
#   Delete vcpkg directory.
#   Follow instructions online to re-install.
# No idea why, but mine stopped working and reinstalling it was the only way to get it working again.
# This also means using vcpkg to reinstall all dependencies.
#
########################################################################################################################

# enable full optimization
if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /fp:fast")
elseif (GNU OR CLANG)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math")
endif ()

########################################################################################################################
# this project
########################################################################################################################

file(GLOB cpps src/*.cpp util/*.cpp)
add_executable(${PROJECT_NAME} ${cpps})
target_include_directories(${PROJECT_NAME} PRIVATE util)

# enable exceptions (off by default on some compilers)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -EHsc")

########################################################################################################################
# create symlink to assets/ directory in build directory
########################################################################################################################

# thanks: https://www.reddit.com/r/cmake/comments/ms89nx/comment/gus0nqx
# Set a symbolic link so exe can find the /assets folder
set(link_src "${CMAKE_SOURCE_DIR}/assets")
if (APPLE)
    # dunno why APPLE is different, maybe it's not (haven't tested yet)
    set(link_dst "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assets")
else ()
    set(link_dst "${CMAKE_CURRENT_BINARY_DIR}/assets")
endif ()
add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD COMMAND ${CMAKE_COMMAND}
        -E create_symlink ${link_src} ${link_dst}
        DEPENDS ${link_dst}
        COMMENT "symbolic link ${link_src} => ${link_dst}"
)

########################################################################################################################
# libraries
########################################################################################################################

# SDL2
#   vcpkg install sdl2:x64-windows
find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE "SDL2::SDL2;SDL2::SDL2main")

# SDL2-image
#   vcpkg install sdl2-image:x64-windows
find_package(sdl2-image CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE "SDL2::SDL2_image")

# OpenGL Mathematics (glm)
find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE "glm::glm")

########################################################################################################################
# for later maybe
########################################################################################################################

#target_compile_definitions(${PROJECT_NAME} PUBLIC TRANSPARENT_WINDOW )
