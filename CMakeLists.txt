cmake_minimum_required(VERSION 3.0...3.25)
project(rengpu
    VERSION 0.1.0
    LANGUAGES CXX C
)

option ( TRACY_ENABLE " " ON )
option ( TRACY_ON_DEMAND " " ON )

add_subdirectory(vendor)
add_subdirectory(src)

