cmake_minimum_required(VERSION 2.6)
project(cam2video)

add_executable(cam2video main.cpp)

install(TARGETS cam2video RUNTIME DESTINATION bin)
