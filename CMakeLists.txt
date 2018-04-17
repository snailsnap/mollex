cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(musselize LANGUAGES CXX)

find_package(OpenCV REQUIRED)

add_subdirectory(src)
