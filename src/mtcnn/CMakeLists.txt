aux_source_directory(. srcs)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/include/ncnn
  /home/user/cv22/pose/ncnn/src
  /home/user/cv22/pose/ncnn/build-aarch64-linux-gnu/src
)

link_directories(${PROJECT_SOURCE_DIR}/lib)

SET(NCNN_LIBS /home/user/cv22/pose/ncnn/build-aarch64-linux-gnu/src/libncnn.a)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

add_library(mtcnn SHARED ${srcs})
add_library(mtcnn_static STATIC ${srcs})

target_link_libraries(mtcnn ${NCNN_LIBS})
target_link_libraries(mtcnn_static ${NCNN_LIBS} m)

set_target_properties(mtcnn_static PROPERTIES OUTPUT_NAME "mtcnn")
set_target_properties(mtcnn PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(mtcnn PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(mtcnn PROPERTIES VERSION 1.0 SOVERSION 1)
