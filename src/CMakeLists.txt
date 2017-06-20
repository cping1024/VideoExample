cmake_minimum_required(VERSION 2.8)
file(GLOB_RECURSE cpp_src *.cpp)
file(GLOB_RECURSE cu_src *.cu)
CUDA_COMPILE(CUDA_OBJS ${cu_src})

add_library(sn_codec_gpu SHARED ${cpp_src} ${CUDA_OBJS})
target_link_libraries(sn_codec_gpu ${common_deps} ${ffmpeg_deps} ${nvcodec_deps} cuda)
set_target_properties(sn_codec_gpu PROPERTIES VERSION ${CUCODEC_VERSION} SVERSION ${CUCODEC_MAJOR_VERSION})
install(TARGETS sn_codec_gpu DESTINATION lib)
