cmake_minimum_required(VERSION 3.5)
project(sdl-metal)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(CMAKE_CXX_STANDARD 17)

option(USE_SDL3
  "Set to true to build the SDL3 version of this demo app" FALSE
)


if(USE_SDL3)
  find_package(SDL3 REQUIRED)
  set(SDL_INCLUDE_DIRS "${SDL3_INCLUDE_DIRS}")
  set(SDL_LIBRARIES "${SDL3_LIBRARIES}")
  set(sdl_metal_SOURCES main-SDL3.cpp)
else()
  find_package(SDL2 REQUIRED)
  set(SDL_INCLUDE_DIRS "${SDL2_INCLUDE_DIRS}")
  set(SDL_LIBRARIES "${SDL2_LIBRARIES}")
  set(sdl_metal_SOURCES main.cpp)
endif()

include(metal)

add_subdirectory(metal-cpp)
add_compiled_metal_sources(sdl_metal_SOURCES triangle.metal)

add_executable(sdl-metal ${sdl_metal_SOURCES})

target_include_directories(
    sdl-metal
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
            "${SDL_INCLUDE_DIRS}"
            "${CMAKE_SOURCE_DIR}/metal-cpp")

target_link_libraries(
    sdl-metal
    PRIVATE "${SDL_LIBRARIES}" MetalCPP "-framework Metal" "-framework QuartzCore" "-framework Foundation")
