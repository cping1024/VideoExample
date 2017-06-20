#ifndef __SN_CUVID_DECODER_H_
#define __SN_CUVID_DECODER_H_

#include <nvcuvid.h>
#include <stdint.h>
#include <vector>
#include <assert.h>
#include <mutex>

namespace SN_DECODER{

enum NvCodec {
    NvCodec_MPEG1 = 0,
    NvCodec_MPEG2 = 1,
    NvCodec_MPEG4 = 2,
    NvCodec_H264  = 3
};

enum NvChromaFormat {
    NvChromaFormat_420 = 0,
    NvChromaFormat_422 = 1,
    NvChromaFormat_444 = 2
};

enum NvColorSpace {
    NvColor_BGRA   = 0,
    NvColor_BGR    = 1,
    NvColor_YUV    = 2,
    NvColor_YUVA   = 3
};

class SNCuvidDecoder {
public:
    SNCuvidDecoder(std::mutex* _pMutex = NULL) : pMutex(_pMutex) {}
    ~SNCuvidDecoder(){}

    CUcontext GetContext() const {return cuContext;}
    std::mutex *GetMutex() const {return pMutex;}

    int GetWidth() const {assert(nWidth); return nWidth;}
    int GetHeight() const {assert(nHeight); return nHeight;}

    bool Create(int codec, int device);
    bool Release();

    bool Decode(const uint8_t *pData, int nSize, uint8_t *pOut, int *outsize, int ofmt);
    bool GetDecodedFrame(uint8_t* pFrame, int *outsize, int ofmt);

private:
    static int CUDAAPI HandleVideoSequenceProc(void *pUserData, CUVIDEOFORMAT *pVideoFormat) {return ((SNCuvidDecoder *)pUserData)->HandleVideoSequence(pVideoFormat);}
    static int CUDAAPI HandlePictureDecodeProc(void *pUserData, CUVIDPICPARAMS *pPicParams) {return ((SNCuvidDecoder *)pUserData)->HandlePictureDecode(pPicParams);}
    static int CUDAAPI HandlePictureDisplayProc(void *pUserData, CUVIDPARSERDISPINFO *pDispInfo) {return ((SNCuvidDecoder *)pUserData)->HandlePictureDisplay(pDispInfo);}
    int HandleVideoSequence(CUVIDEOFORMAT *pVideoFormat);
    int HandlePictureDecode(CUVIDPICPARAMS *pPicParams);
    int HandlePictureDisplay(CUVIDPARSERDISPINFO *pDispInfo);

private:
    CUcontext cuContext = NULL;
    CUvideoctxlock ctxLock;
    std::mutex *pMutex;
    CUvideoparser hParser = NULL;
    CUvideodecoder hDecoder = NULL;
    bool bUseDeviceMemoryFrame;
    unsigned int nWidth = 0, nHeight = 0, nSrcPitch = 0;
    cudaVideoCodec eCodec = cudaVideoCodec_NumCodecs;
    cudaVideoChromaFormat eChromaFormat;
    // state of vpFrame
    int nDecodedFrame = 0, nDecodedFrameReturned = 0;
    // raw bitstream cache
    bool bEndDecodeDone = false;

    unsigned char* bgraData = NULL;
    unsigned char* tempData = NULL;
};


} //NAMESPACE

#endif //__SN_CUVID_DECODER_H_
