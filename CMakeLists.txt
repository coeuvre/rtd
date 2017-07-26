cmake_minimum_required(VERSION 3.8)
project(rtd)

set(CMAKE_C_STANDARD 99)

add_executable(rtd src/main.c)
target_link_libraries(rtd mingw32 SDL2main SDL2)