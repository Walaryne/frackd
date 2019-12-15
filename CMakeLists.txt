cmake_minimum_required(VERSION 3.10)

project(frackd LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(EXECUTABLE_OUTPUT_PATH ./${CMAKE_BUILD_TYPE})

add_executable(frackd src/frackd.c)
target_compile_options(frackd PUBLIC -pedantic -DUSE_SYSLOG)

set_property(TARGET frackd PROPERTY C_STANDARD 11)
set_property(TARGET frackd PROPERTY C_EXTENSIONS ON)
