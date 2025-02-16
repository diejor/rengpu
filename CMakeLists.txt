cmake_minimum_required(VERSION 3.0...3.25)
project(LearnWebGPU
    VERSION 0.1.0
    LANGUAGES CXX C
)

if (NOT EMSCRIPTEN)
    add_subdirectory(glfw)
else()
    add_library(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()
# add glfw before webgpu so the version can be decided

add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)
add_subdirectory(src)

