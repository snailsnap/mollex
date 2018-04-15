cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
project(musselize LANGUAGES CXX C)

add_executable(musselize musselize.cxx)
target_link_libraries(musselize PRIVATE opencv_core opencv_imgcodecs)
