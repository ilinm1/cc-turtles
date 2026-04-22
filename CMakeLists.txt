cmake_minimum_required(VERSION 3.5.0)
project(TurtleUtilities VERSION 0.1.0 LANGUAGES C CXX)
set (CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
include_directories(lib/stb src)

add_executable(img2vox src/img2vox.cpp lib/stb/stb_image.c)
add_executable(vox2bin src/vox2bin.cpp src/turtle.cpp)
