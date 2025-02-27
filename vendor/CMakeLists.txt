if (NOT EMSCRIPTEN)
    add_subdirectory(glfw)
else()
    add_library(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()
# add glfw before webgpu so the version can be decided

add_subdirectory(tracy)
add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)
add_subdirectory(imgui)
