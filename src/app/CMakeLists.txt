add_executable(app 
    Main.cpp 
    Application.cpp 
)


set_target_properties(app PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNINGS_AS_ERRORS ON
)

if (MSVC)
    target_compile_options(app PRIVATE /W4)
else ()
    target_compile_options(app PRIVATE -Wall -Wextra -pedantic)
endif ()

target_include_directories(app PRIVATE
    Application.hpp
    ${CMAKE_SOURCE_DIR}/vendor/glm
)

# include directories here
target_link_libraries(app PRIVATE
    webgpu
    glfw
    glfw3webgpu
    renderer
    utils
    imgui
    Tracy::TracyClient
)


# install shared libraries next to the binary on Linux
if(UNIX AND NOT APPLE)
    set_target_properties(app PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()


if (XCODE)
    set_target_properties(app PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
    )
endif()

if (EMSCRIPTEN)
    set(SHELL_FILE shell_minimal.html)

    target_link_options(app PRIVATE
        -sUSE_GLFW=3
        -sUSE_WEBGPU
        -sASYNCIFY
        --preload-file "${CMAKE_SOURCE_DIR}/resources@/resources"
        --shell-file "${CMAKE_SOURCE_DIR}/resources/${SHELL_FILE}"
        -sALLOW_MEMORY_GROWTH
    )

    set_property(
        TARGET app
        PROPERTY LINK_DEPENDS
        "${CMAKE_SOURCE_DIR}/resources/${SHELL_FILE}"
    )

    set_target_properties(app PROPERTIES 
        OUTPUT_NAME "index"
        SUFFIX ".html" 
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

endif()

# The application's binary must find wgpu.dll or libwgpu.so at runtime,
# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
# next to the binary.
target_copy_webgpu_binaries(app)

install(TARGETS app)
