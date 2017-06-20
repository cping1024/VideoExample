#ifndef __CUVIDE_CODEC_UTIL_H_
#define __CUVIDE_CODEC_UTIL_H_

#include <chrono>
#include <sys/stat.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <util/logger.h>

//#ifdef __cuda_cuda_h__
inline bool check(CUresult e, int iLine, const char *szFile) {
    if (e != CUDA_SUCCESS) {
        LOG_ERROR(logger, "CUDA error " << e << " at line " << iLine << " in file " << szFile);
        return false;
    }
    return true;
}
//#endif

#ifdef __CUDA_RUNTIME_H__
inline bool check(cudaError_t e, int iLine, const char *szFile) {
    if (e != cudaSuccess) {
        LOG_ERROR(logger, "CUDA runtime error " << e << " at line " << iLine << " in file " << szFile);
        return false;
    }
    return true;
}
#endif

#ifdef _NV_ENCODEAPI_H_
inline bool check(NVENCSTATUS e, int iLine, const char *szFile) {
    if (e != NV_ENC_SUCCESS) {
        LOG_ERROR(logger, "NVENC error " << e << " at line " << iLine << " in file " << szFile);
        return false;
    }
    return true;
}
#endif

#define cuSafeCall(call) do { \
	check(call, __LINE__, __FILE__);\
} while(0);

#endif //_CUVIDE_CODEC_UTIL_H_
