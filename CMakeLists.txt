cmake_minimum_required(VERSION 3.0...3.25)
project(LearnWebGPU
    VERSION 0.1.0
    LANGUAGES CXX C
)

add_subdirectory(webgpu)
add_subdirectory(SDL2)
add_subdirectory(sdl2webgpu)
add_subdirectory(src)

