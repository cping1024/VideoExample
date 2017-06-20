cmake_minimum_required(VERSION 2.8)

find_package(CUDA 7.5 REQUIRED)
include_directories(system ${CUDA_INCLUDE_DIRS})
list(APPEND common_deps ${CUDA_LIBRARIES})

#threads
find_package(Threads REQUIRED)
list(APPEND common_deps ${CMAKE_THREAD_LIBS_INIT})

#live555
file(GLOB_RECURSE LIVEMEDIA_DEPS ${PROJECT_SOURCE_DIR}/deps/live555/lib/*.a)
list(APPEND common_deps ${LIVEMEDIA_DEPS})
include_directories(system ${PROJECT_SOURCE_DIR}/deps/live555/include)
install(FILES ${LIVEMEDIA_DEPS} DESTINATION lib)

#nvcodec
#list(APPEND nvcodec_deps /usr/lib/nvidia-375/libnvcuvid.so)
file(GLOB_RECURSE NV_CODEC_LIBS ${PROJECT_SOURCE_DIR}/deps/nvCodec/lib/*.so)
#include_directories(system ${PROJECT_SOURCE_DIR}/deps/nvCodec/include)
list(APPEND common_deps ${NV_CODEC_LIBS})

file(GLOB_RECURSE NV_CODEC_INSTALL_LIBS ${PROJECT_SOURCE_DIR}/deps/nvCodec/lib/*)
install(FILES ${NV_CODEC_INSTALL_LIBS} DESTINATION lib)

#deepstream
#file(GLOB_RECURSE NV_DEEPSTREAM_LIBS ${PROJECT_SOURCE_DIR}/deps/deepStream/lib/*.so)
#include_directories(system ${PROJECT_SOURCE_DIR}/deps/deepStream/include)
#list(APPEND deepstream_deps ${NV_DEEPSTREAM_LIBS})

#file(GLOB_RECURSE NV_DEEPSTREAM_INSTALL_LIBS ${PROJECT_SOURCE_DIR}/deps/deepStream/lib/*)
#install(FILES ${NV_DEEPSTREAM_INSTALL_LIBS} DESTINATION lib)

#ffmpeg libavformat
file(GLOB_RECURSE FFMPEG_LIBS ${PROJECT_SOURCE_DIR}/deps/ffmpeg/lib/*.so)
include_directories(system ${PROJECT_SOURCE_DIR}/deps/ffmpeg/include)
list(APPEND ffmpeg_deps ${PROJECT_SOURCE_DIR}/deps/ffmpeg/lib/libavformat.so)
list(APPEND ffmpeg_deps ${PROJECT_SOURCE_DIR}/deps/ffmpeg/lib/libavcodec.so)

file(GLOB_RECURSE FFMPEG_INSTALL_LIBS ${PROJECT_SOURCE_DIR}/deps/ffmpeg/lib/*)
install(FILES ${FFMPEG_INSTALL_LIBS} DESTINATION lib)

