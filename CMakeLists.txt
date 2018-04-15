cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
project(musselize LANGUAGES CXX C)

find_package(OpenCV REQUIRED)

add_subdirectory(src)
