#1.cmake verson
cmake_minimum_required(VERSION 2.8)

#2.project name
project(mtcnn_ncnn C CXX)

#3.set environment variable
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(src)