cmake_minimum_required(VERSION 3.14)

project(vaq-make VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

add_executable(vaq-make src/scanner.c src/vaq-make.c src/generator.c)
target_include_directories(
  vaq-make
  PUBLIC include/
  PRIVATE private/)
