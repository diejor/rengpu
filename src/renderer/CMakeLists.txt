add_library(renderer STATIC
    RendererContext.hpp
    RendererContext.cpp
    RendererSurface.hpp  
    RendererDriver.hpp
    RendererDriver.cpp
)

if (NOT EMSCRIPTEN)
    target_compile_definitions(renderer PRIVATE
        RESOURCE_DIR="${CMAKE_SOURCE_DIR}/resources/"
    )
else ()
    target_compile_definitions(renderer PRIVATE
        RESOURCE_DIR="/resources/"
    )
endif ()

set_target_properties(renderer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNINGS_AS_ERRORS ON
)

target_link_libraries(renderer PRIVATE
    Tracy::TracyClient
    webgpu
    glfw3webgpu
    glfw
    utils
    app
)
