cmake_minimum_required(VERSION 3.20)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

########################################################################################################################
# make sure vcpkg git submodule is downloaded
########################################################################################################################

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/.git")
    message(FATAL_ERROR "The git submodule vcpkg is not available. Please run:\ngit submodule update --init --recursive")
endif ()

# configure CMake to find vcpkg libraries
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")

########################################################################################################################
# create the project
########################################################################################################################

project(2d-experiments CXX)

# TODO: make sure SSE / AVX are enabled on appropriate platforms

# enable full optimization
if (MSVC)
    # NOTE: clang-cl also detected as MSVC which is not ideal because it supports different arguments
    message("MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /fp:fast /arch:AVX2 /GS-")
elseif (GNU OR CLANG OR APPLE)
    # NOTE: APPLE not detected as CLANG, even though it is using clang
    message("GNU OR CLANG OR APPLE")
    # NOTE: not sure if -mcpu=apple-m1 has any effect, it doesn't seem to affect performance of the compiled executable
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fno-trapping-math -fno-math-errno -fno-signed-zeros -ftree-vectorize")
endif ()

# enable exceptions (off by default on clang-cl at least, which unfortunately detects as MSVC)
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif ()

file(GLOB cpps src/*.cpp util/*.cpp)
add_executable(${PROJECT_NAME} ${cpps})
target_include_directories(${PROJECT_NAME} PRIVATE util)

########################################################################################################################
# tell C++ source about compiling for Apple
########################################################################################################################

if (APPLE)
    message("detected Apple platform")
    target_compile_definitions(${PROJECT_NAME} PUBLIC APPLE)
endif ()

########################################################################################################################
# create symlink to assets/ directory in build directory
########################################################################################################################

# Set a symbolic link so exe can find the /assets folder.
# (During a release build the assets directory will need to be copied; this script doesn't do that yet).
# thanks: https://www.reddit.com/r/cmake/comments/ms89nx/comment/gus0nqx
set(link_src "${CMAKE_SOURCE_DIR}/assets")
set(link_dst "${CMAKE_CURRENT_BINARY_DIR}/assets")
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

# OpenGL Mathematics (glm)
find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE "glm::glm")

# SDL2
find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE "SDL2::SDL2;SDL2::SDL2main")

# SDL2-image
find_package(sdl2-image CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE "SDL2::SDL2_image")

########################################################################################################################
# for later maybe
########################################################################################################################

#target_compile_definitions(${PROJECT_NAME} PUBLIC TRANSPARENT_WINDOW )
