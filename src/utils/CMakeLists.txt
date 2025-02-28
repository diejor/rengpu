add_library(utils INTERFACE)

set_target_properties(utils PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNINGS_AS_ERRORS ON
)

if (MSVC)
    target_compile_options(utils INTERFACE /W4 /Zc:preprocessor)
endif()

target_include_directories(utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
