add_executable(app main.cpp)

set_target_properties(app PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNINGS_AS_ERRORS ON
)

if(UNIX AND NOT APPLE)
    set_target_properties(app PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()

if (MSVC)
    target_compile_options(app PRIVATE /W4)
else ()
    target_compile_options(app PRIVATE -Wall -Wextra -pedantic)
endif ()

if (XCODE)
    set_target_properties(app PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
    )
endif()

if (NOT EMSCRIPTEN)
    add_subdirectory(glfw)
else()
    add_library(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()
# add glfw before webgpu so the version can be decided

# include directories here
target_link_libraries(app PRIVATE
    utils
    webgpu
    glfw
)

if (EMSCRIPTEN)
    set_target_properties(app PROPERTIES SUFFIX ".html")
    target_link_options(app PRIVATE -sASYNCIFY)
endif()


# The application's binary must find wgpu.dll or libwgpu.so at runtime,
# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
# next to the binary.
target_copy_webgpu_binaries(app)

install(TARGETS app)
