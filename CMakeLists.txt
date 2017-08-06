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


add_executable(char2hex src/tools/char2hex.c)

set(shaders)
file(GLOB shaders_source "src/shaders/*")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
foreach(input ${shaders_source})
    get_filename_component(filename ${input} NAME)
    set(output "${CMAKE_BINARY_DIR}/shaders/${filename}.gen")
    add_custom_command(OUTPUT ${output}
                       DEPENDS char2hex ${input}
                       COMMAND char2hex ${output} ${input})
    list(APPEND shaders ${output})
endforeach ()

include_directories(${CMAKE_BINARY_DIR})


include_directories(libs/stb)

add_executable(rtd src/main.c ${shaders})

if (MINGW)
    target_link_libraries(rtd glad mingw32 SDL2main SDL2)
elseif(WIN32)
    target_link_libraries(rtd glad SDL2main SDL2)
endif()