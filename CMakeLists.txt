cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 14)
project(Simple)

add_library(simple INTERFACE)
target_include_directories(simple INTERFACE include/)

add_executable(main src/main.cpp)
target_link_libraries(main simple)





