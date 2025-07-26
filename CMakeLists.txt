cmake_minimum_required(VERSION 3.14)

project(vaq-make VERSION 0.2.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

add_executable(
  vaq-make
  src/native/class.c
  src/native/fun.c
  src/array.c
  src/file.c
  src/generator.c
  src/object.c
  src/scanner.c
  src/table.c
  src/value.c
  src/vaq-make.c)
target_include_directories(
  vaq-make
  PUBLIC include/
  PRIVATE private/)
target_link_libraries(vaq-make m)
