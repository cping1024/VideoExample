cmake_minimum_required(VERSION 2.8)
project(cucodec C CXX)

SET(CUCODEC_MAJOR_VERSION 1)
SET(CUCODEC_MINOR_VERSION 0)
SET(CUCODEC_PATCH_VERSION 0)

SET(CUCODEC_VERSION ${CUCODEC_MAJOR_VERSION}.${CUCODEC_MINOR_VERSION}.${CUCODEC_PATCH_VERSION})

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fPIC -std=c++11")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-fPIC")
SET(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} "-g -Xcompiler -fPIC -std=c++11
    -gencode=arch=compute_20,code=\"sm_20\"
    -gencode=arch=compute_20,code=\"sm_21\"
    -gencode=arch=compute_30,code=\"sm_30\"
    -gencode=arch=compute_35,code=\"sm_35\"
    -gencode=arch=compute_50,code=\"sm_50\"
    -gencode=arch=compute_50,code=\"compute_50\"
    ")

set(common_deps "")
set(nvcodec_deps "")
set(ffmpeg_deps "")
#set(deepstream_deps "")

include(${PROJECT_SOURCE_DIR}/deps/deps.cmake)
include_directories(include)

add_subdirectory(src)
add_subdirectory(example)

install(DIRECTORY models DESTINATION .)
