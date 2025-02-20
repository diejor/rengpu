cmake_minimum_required(VERSION 3.0.0...3.24 FATAL_ERROR)
message(STATUS "CMake version: ${CMAKE_VERSION}")

if (EMSCRIPTEN)
    add_library(webgpu INTERFACE)
    target_compile_definitions(webgpu INTERFACE
        WEBGPU_BACKEND_EMSCRIPTEN
    )
    target_link_options(webgpu INTERFACE 
        "SHELL:-s USE_WEBGPU=1"
    )
else (EMSCRIPTEN)
    set(WGPU_NATIVE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
    set(WGPU_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
    message(STATUS "WGPU_INCLUDE_DIR: ${WGPU_INCLUDE_DIR}")
    message(STATUS "WGPU_NATIVE_LIB_DIR: ${WGPU_NATIVE_LIB_DIR}")

    find_library(
        WGPU_LIBRARY NAMES libwgpu_native.a wgpu_native.lib wgpu_nativeo
        HINTS ${WGPU_NATIVE_LIB_DIR}
        REQUIRED
    )
    add_library(webgpu STATIC IMPORTED GLOBAL)
    set_target_properties(webgpu PROPERTIES
        IMPORTED_LOCATION "${WGPU_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${WGPU_INCLUDE_DIR}"
    )
    target_compile_definitions(webgpu INTERFACE WEBGPU_BACKEND_WGPU)



endif (EMSCRIPTEN)
