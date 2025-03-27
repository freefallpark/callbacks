cmake_minimum_required(VERSION 3.29)
project(callbacks)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(tools)

add_executable(callbacks main.cpp)
target_link_libraries(callbacks CallbackTools)
