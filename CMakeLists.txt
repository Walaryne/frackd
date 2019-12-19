cmake_minimum_required(VERSION 3.10)

project(frackd LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(frackd src/frackd.c)
target_compile_options(frackd PUBLIC -pedantic)

set_property(TARGET frackd PROPERTY C_STANDARD 11)
set_property(TARGET frackd PROPERTY C_EXTENSIONS ON)
