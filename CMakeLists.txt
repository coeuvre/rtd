cmake_minimum_required(VERSION 3.8)

project(rtd)

set(CMAKE_C_STANDARD 99)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

# Setup libraries
if (CMAKE_BUILD_TYPE MATCHES Debug)
    include_directories(lib/glad/debug/include)
    add_library(glad lib/glad/debug/src/glad.c)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    include_directories(lib/glad/release/include)
    add_library(glad lib/glad/release/src/glad.c)
endif ()

include_directories(lib/stb)

# Compile tool char2hex
add_executable(char2hex src/tools/char2hex.c)

# Compile shaders
set(shaders)
set(shaders_source
        src/shaders/draw_texture.frag
        src/shaders/draw_texture.vert)
foreach (shader_source ${shaders_source})
    set(input ${CMAKE_SOURCE_DIR}/${shader_source})
    set(output ${input}.gen)
    add_custom_command(OUTPUT ${output}
            DEPENDS char2hex ${input}
            COMMAND char2hex ${output} ${input})
    list(APPEND shaders ${output})
endforeach ()

# Compile rtd
FILE (GLOB_RECURSE clion_all_headers ${CMAKE_SOURCE_DIR}/src/*.h)
add_executable(rtd
        src/image.c
        src/main.c
        src/renderer.c
        src/time.c
        src/window.c
        ${clion_all_headers}
        ${shaders})

set(libs glad)

if (MINGW)
    list(APPEND libs mingw32 SDL2main SDL2)
    add_definitions(-DRTD_WIN32)
elseif (WIN32)
    list(APPEND libs SDL2main SDL2)
    add_definitions(-DRTD_WIN32)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    list(APPEND libs SDL2)
    add_definitions(-DRTD_MACOS)
endif ()

target_link_libraries(rtd ${libs})
