add_library(renderer STATIC
    Context.hpp
    Context.cpp
    Driver.hpp
    Driver.cpp

    Vertex.hpp
)

set_target_properties(renderer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNINGS_AS_ERRORS ON
)

if (MSVC)
    target_compile_options(renderer PRIVATE /W4)
else ()
    target_compile_options(renderer PRIVATE -Wall -Wextra -pedantic)
endif ()

if (NOT EMSCRIPTEN)
    target_compile_definitions(renderer PRIVATE
        RESOURCE_DIR="${CMAKE_SOURCE_DIR}/resources/"
    )
else ()
    target_compile_definitions(renderer PRIVATE
        RESOURCE_DIR="/resources/"
    )
endif ()


target_include_directories(renderer PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/glm
)

target_link_libraries(renderer PRIVATE
    Tracy::TracyClient
    webgpu
    glfw3webgpu
    glfw
    utils
    app
)
