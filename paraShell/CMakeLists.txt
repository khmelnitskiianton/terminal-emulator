#cmake -S . -B build
#cmake --build build

cmake_minimum_required(VERSION 3.10)
project(paraShell)
add_executable(paraShell main.cpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(paraShell VERSION 1.0)
target_compile_options(paraShell PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
target_link_libraries(paraShell PRIVATE -fsanitize=address,undefined)
