cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 14)

project(objectpool_lib)

add_executable(main main.cpp)
add_library(objectpool INTERFACE)
target_include_directories(objectpool INTERFACE include/)
