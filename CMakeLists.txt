cmake_minimum_required(VERSION 3.8)

project(rtd)

set(CMAKE_C_STANDARD 99)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE MATCHES Debug)
    include_directories(libs/glad/debug/include)
    add_library(glad libs/glad/debug/src/glad.c)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    include_directories(libs/glad/release/include)
    add_library(glad libs/glad/release/src/glad.c)
endif ()

include_directories(libs/stb)

add_executable(rtd src/main.c)

target_link_libraries(rtd glad mingw32 SDL2main SDL2)