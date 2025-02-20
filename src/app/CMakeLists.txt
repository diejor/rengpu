add_executable(app 
    main.cpp 
    application.cpp 
    webgpu-utils.cpp
)

set_target_properties(app PROPERTIES
    CXX_STANDARD 20
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

target_link_libraries(app PRIVATE
    utils
    webgpu
    glfw
    glfw-webgpu
)


if (EMSCRIPTEN)
    set(SHELL_FILE shell_minimal.html)

    target_link_options(app PRIVATE
        -sUSE_GLFW=3
        -sUSE_WEBGPU
        -sASYNCIFY
        --preload-file "${CMAKE_SOURCE_DIR}/resources@/resources"
        --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}"
        -sALLOW_MEMORY_GROWTH
    )

    set_property(
        TARGET app
        PROPERTY LINK_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}"
    )

    set_target_properties(app PROPERTIES 
        OUTPUT_NAME "index"
        SUFFIX ".html" 
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

endif()

install(TARGETS app)
