aux_source_directory(. srcs)

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

include_directories(${PROJECT_SOURCE_DIR}/include )
include_directories(${PROJECT_SOURCE_DIR}/include/ncnn )
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

add_executable(mtcnn_test ${srcs})

target_link_libraries(mtcnn_test mtcnn)
#target_link_libraries( mtcnn_test opencv_imgproc opencv_core opencv_highgui)
target_link_libraries( mtcnn_test ${OPENCV_CORE} ${OPENCV_IMGPROC} ${OPENCV_HIGHGUI} m)

