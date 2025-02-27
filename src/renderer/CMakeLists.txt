add_library(renderer STATIC
    RendererContext.hpp
    RendererContext.cpp
    RendererSurface.hpp  
    RendererDriver.hpp
    RendererDriver.cpp
)

target_include_directories(renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(renderer PRIVATE
    Tracy::TracyClient
    webgpu
    glfw3webgpu
    glfw
    utils
    app
)
