cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(musselize LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

add_subdirectory(src)
