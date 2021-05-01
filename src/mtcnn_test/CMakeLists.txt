#
# Copyright © 2020 by UDP Technology Ltd.
#
# All rights reserved. Property of UDP Technology Ltd. Restricted rights to use,
# duplicate or disclose this code are granted through contract.
#

SET(TARGET_NAME mtcnn_test)


#
# Conan
#

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

FIND_PACKAGE( OpenMP REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")  
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")  

set (SDK30_ARMV8_DIR /home/user/cv22/cv22_linux_sdk_3.0/ambarella/prebuild/oss/armv8-a)

include_directories(
  ${SDK30_ARMV8_DIR}/opencv/include
  /home/user/cv22/pose/ncnn/src
  /home/user/cv22/pose/ncnn/build-aarch64-linux-gnu/src
)

find_library(OPENCV_CORE opencv_core ${SDK30_ARMV8_DIR}/opencv/usr/lib)
find_library(OPENCV_IMGCODECS opencv_imgcodecs ${SDK30_ARMV8_DIR}/opencv/usr/lib)
find_library(OPENCV_IMGPROC opencv_imgproc ${SDK30_ARMV8_DIR}/opencv/usr/lib)
find_library(OPENCV_VIDEOIO opencv_videoio ${SDK30_ARMV8_DIR}/opencv/usr/lib)
find_library(OPENCV_HIGHGUI opencv_highgui ${SDK30_ARMV8_DIR}/opencv/usr/lib)

find_library(JPEG jpeg ${SDK30_ARMV8_DIR}/libjpeg-turbo/usr/lib)
find_library(PNG png ${SDK30_ARMV8_DIR}/libpng/usr/lib)
find_library(PROTOBUF protobuf ${SDK30_ARMV8_DIR}/protobuf/usr/lib)
find_library(Z z ${SDK30_ARMV8_DIR}/zlib/usr/lib)

#
# Options
#

option(WITH_FFMPEG "Build with ffmpeg" TRUE)

find_package(PkgConfig)
if(WITH_FFMPEG)
#  pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)
endif()

include_directories(
  ${PROJECT_SOURCE_DIR}/include 
  ${PROJECT_SOURCE_DIR}/include/ncnn
)

if(WITH_FFMPEG)
  set(FFMPEG_SOURCES
    ffmpeg_common.cpp
    ffmpeg_input.cpp
  )
  add_compile_definitions(WITH_FFMPEG)
endif()

link_directories(${PROJECT_SOURCE_DIR}/lib)

link_libraries(
  ${OPENCV_CORE}
  ${OPENCV_IMGCODECS}
  ${OPENCV_IMGPROC}
  ${OPENCV_VIDEOIO}
  ${OPENCV_HIGHGUI}
  ${JPEG}
  ${PNG}
  ${PROTOBUF}
  ${Z}
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(
  SOURCES
  example_face.cpp
  stream_output.cpp
  subprocess_output.cpp
  test_input.cpp
  video_input.cpp
  video_output.cpp
  ${FFMPEG_SOURCES}
)

add_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(${TARGET_NAME}
  ${CONAN_LIBS}
  m
  mtcnn
  ${OPENCV_CORE}
  ${OPENCV_HIGHGUI}
  ${OPENCV_IMGPROC}
)

