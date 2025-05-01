cmake_minimum_required(VERSION 3.10)
project(Shell)
add_executable(Shell main.cpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Shell VERSION 1.0)
configure_file(ShellConfig.h.in ShellConfig.h)
target_include_directories(Shell PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )
target_compile_options(Shell PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
target_link_libraries(Shell PRIVATE -fsanitize=address,undefined)
