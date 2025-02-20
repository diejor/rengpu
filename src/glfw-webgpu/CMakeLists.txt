add_library(glfw-webgpu STATIC glfw-webgpu.c)
target_include_directories(glfw-webgpu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(glfw-webgpu PRIVATE
    webgpu
    glfw
)

if (GLFW_BUILD_WIN32)
    target_compile_definitions(glfw-webgpu PRIVATE _GLFW_WIN32)
endif()
if (GLFW_BUILD_COCOA)
    target_compile_definitions(glfw-webgpu PRIVATE _GLFW_COCOA)
    target_link_libraries(glfw-webgpu PRIVATE "-framework Cocoa" "-framework CoreVideo" "-framework IOKit" "-framework QuartzCore")
    target_compile_options(glfw-webgpu PRIVATE -x objective-c)
endif()
if (GLFW_BUILD_WAYLAND)
    target_compile_definitions(glfw-webgpu PRIVATE _GLFW_WAYLAND)
endif()
if (GLFW_BUILD_X11)
    target_compile_definitions(glfw-webgpu PRIVATE _GLFW_X11)
endif()
