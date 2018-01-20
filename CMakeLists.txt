cmake_minimum_required(VERSION 2.6)
project(cam2video)

add_executable(cam2video main.cpp)

set(CMAKE_CXX_FLAGS "-std=c++11")

target_link_libraries(cam2video avutil avcodec avformat swscale avdevice pthread)

#install(TARGETS cam2video RUNTIME DESTINATION bin)
