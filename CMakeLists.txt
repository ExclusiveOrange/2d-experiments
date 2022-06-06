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

# tell compiler to generate assembler listings
if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /FAsu")
endif ()

# tell compiler to enable auto vectorization report
if (MSVC)
    # Qvec-report:1 successfully vectorized loops only
    # Qvec-report:2 successfully and unsuccessfully vectorized loops
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Qvec-report:2")
endif ()

# enable full optimization
if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /fp:fast")
endif()

########################################################################################################################
# external libraries
########################################################################################################################

# SDL2
#   (from vcpkg directory, not this project directory)
#   vcpkg install sdl2:x64-windows
find_package(SDL2 CONFIG REQUIRED)

########################################################################################################################
# this project
########################################################################################################################

file(GLOB cpps src/*.cpp util/*.cpp)

add_executable(${PROJECT_NAME} ${cpps})

# copied from another project in case I need to add more include directories later
#target_include_directories(${PROJECT_NAME} PRIVATE sub/stb)
#target_include_directories(${PROJECT_NAME} PRIVATE util)

target_link_libraries(${PROJECT_NAME} PRIVATE "SDL2::SDL2;SDL2::SDL2main")

#target_compile_definitions(${PROJECT_NAME} PUBLIC TRANSPARENT_WINDOW )
