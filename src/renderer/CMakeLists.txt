add_library(renderer STATIC
    renderer_context.h
    renderer_context.cpp
)

target_include_directories(renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(renderer PRIVATE
    webgpu
    glfw3webgpu
    glfw
    utils
    app
)
