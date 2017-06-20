#include <decoder/sn_cuvid_decoder.h>
#include <iostream>
#include <cuda_runtime.h>
#include <util/cuvid_codec_util.h>
#include <util/colorspace.h>
#include <algorithm>

namespace SN_DECODER{

#define N_DECODE_SURFACE 4

static const char * GetVideoCodecString(cudaVideoCodec eCodec) {
    static struct {
        cudaVideoCodec eCodec;
        const char *name;
    } aCodecName [] = {
        { cudaVideoCodec_MPEG1,    "MPEG-1"     },
        { cudaVideoCodec_MPEG2,    "MPEG-2"     },
        { cudaVideoCodec_MPEG4,    "MPEG-4 (ASP)"},
        { cudaVideoCodec_VC1,      "VC-1/WMV"   },
        { cudaVideoCodec_H264,     "AVC/H.264"  },
        { cudaVideoCodec_JPEG,     "M-JPEG"     },
        { cudaVideoCodec_H264_SVC, "H.264/SVC"  },
        { cudaVideoCodec_H264_MVC, "H.264/MVC"  },
        { cudaVideoCodec_HEVC,     "H.265/HEVC" },
        ///low version nvidia driver don't support
        //{ cudaVideoCodec_VP8,      "VP8"        },
        //{ cudaVideoCodec_VP9,      "VP9"        },
        { cudaVideoCodec_NumCodecs,"Invalid"    },
        { cudaVideoCodec_YUV420,   "YUV  4:2:0" },
        { cudaVideoCodec_YV12,     "YV12 4:2:0" },
        { cudaVideoCodec_NV12,     "NV12 4:2:0" },
        { cudaVideoCodec_YUYV,     "YUYV 4:2:2" },
        { cudaVideoCodec_UYVY,     "UYVY 4:2:2" },
    };

    if (eCodec >= 0 && eCodec <= cudaVideoCodec_NumCodecs) {
        return aCodecName[eCodec].name;
    }
    for (int i = cudaVideoCodec_NumCodecs + 1; i < sizeof(aCodecName) / sizeof(aCodecName[0]); i++) {
        if (eCodec == aCodecName[i].eCodec) {
            return aCodecName[eCodec].name;
        }
    }
    return "Unknown";
}

static const char * GetVideoChromaFormatString(cudaVideoChromaFormat eChromaFormat) {
    static struct {
        cudaVideoChromaFormat eChromaFormat;
        const char *name;
    } aChromaFormatName[] = {
        { cudaVideoChromaFormat_Monochrome, "YUV 400 (Monochrome)" },
        { cudaVideoChromaFormat_420,        "YUV 420"              },
        { cudaVideoChromaFormat_422,        "YUV 422"              },
        { cudaVideoChromaFormat_444,        "YUV 444"              },
    };

    if (eChromaFormat >= 0 && eChromaFormat < sizeof(aChromaFormatName) / sizeof(aChromaFormatName[0])) {
        return aChromaFormatName[eChromaFormat].name;
    }
    return "Unknown";
}

int SNCuvidDecoder::HandleVideoSequence(CUVIDEOFORMAT *pVideoFormat) {
    LOG_INFO(logger, "Video Input Information" << endl
        << "\tVideo codec     : " << GetVideoCodecString(pVideoFormat->codec) << endl
        << "\tFrame rate      : " << pVideoFormat->frame_rate.numerator << "/" << pVideoFormat->frame_rate.denominator
            << " = " << 1.0 * pVideoFormat->frame_rate.numerator / pVideoFormat->frame_rate.denominator << " fps" << endl
        << "\tSequence format : " << (pVideoFormat->progressive_sequence ? "Progressive" : "Interlaced") << endl
        << "\tCoded frame size: [" << pVideoFormat->coded_width << ", " << pVideoFormat->coded_height << "]" << endl
        << "\tDisplay area    : [" << pVideoFormat->display_area.left << ", " << pVideoFormat->display_area.top << ", "
            << pVideoFormat->display_area.right << ", " << pVideoFormat->display_area.bottom << "]" << endl
        << "\tChroma format   : " << GetVideoChromaFormatString(pVideoFormat->chroma_format));

    nWidth = pVideoFormat->display_area.right - pVideoFormat->display_area.left;
    nHeight = pVideoFormat->display_area.bottom - pVideoFormat->display_area.top;
    eCodec = pVideoFormat->codec;
    eChromaFormat = pVideoFormat->chroma_format;

    std::cout << "codec name:" << GetVideoCodecString(pVideoFormat->codec) << std::endl;
    std::cout << "codec width:" << nWidth << ", height:" << nHeight << std::endl;
    std::cout << "chroma format:" << GetVideoChromaFormatString(pVideoFormat->chroma_format) << std::endl;

    CUVIDDECODECREATEINFO videoDecodeCreateInfo = { 0 };
    videoDecodeCreateInfo.CodecType = pVideoFormat->codec;
    videoDecodeCreateInfo.ulWidth = pVideoFormat->coded_width;
    videoDecodeCreateInfo.ulHeight = pVideoFormat->coded_height;
    videoDecodeCreateInfo.ChromaFormat = pVideoFormat->chroma_format;
    videoDecodeCreateInfo.OutputFormat = cudaVideoSurfaceFormat_NV12;
    videoDecodeCreateInfo.DeinterlaceMode = cudaVideoDeinterlaceMode_Adaptive;
    videoDecodeCreateInfo.ulTargetWidth = nWidth;
    videoDecodeCreateInfo.ulTargetHeight = nHeight;
    videoDecodeCreateInfo.ulNumOutputSurfaces = 1;
    videoDecodeCreateInfo.ulCreationFlags = cudaVideoCreate_PreferCUVID;
    videoDecodeCreateInfo.ulNumDecodeSurfaces = N_DECODE_SURFACE;
    videoDecodeCreateInfo.vidLock = ctxLock;
    cuSafeCall(cuCtxPushCurrent(cuContext));
    cuSafeCall(cuvidCreateDecoder(&hDecoder, &videoDecodeCreateInfo));
    cuSafeCall(cuCtxPopCurrent(NULL));
    return 1;
}

int SNCuvidDecoder::HandlePictureDecode(CUVIDPICPARAMS *pPicParams) {
    LOG_DEBUG(logger, "HandlePictureDecode");
    if (!hDecoder) {
        LOG_ERROR(logger, "Decoder not initialized.");
        return false;
    }

    cuSafeCall(cuvidDecodePicture(hDecoder, pPicParams));
    return 1;
}

int SNCuvidDecoder::HandlePictureDisplay(CUVIDPARSERDISPINFO *pDispInfo) {
    LOG_DEBUG(logger, "HandlePictureDisplay");
    nDecodedFrame++;
    CUVIDPROCPARAMS videoProcessingParameters = {};
    videoProcessingParameters.progressive_frame = pDispInfo->progressive_frame;
    videoProcessingParameters.second_field = 0;
    videoProcessingParameters.top_field_first = pDispInfo->top_field_first;
    videoProcessingParameters.unpaired_field = pDispInfo->progressive_frame == 1;

    CUdeviceptr d_pSrcData = 0;
    nSrcPitch = 0;
    cuSafeCall(cuvidMapVideoFrame(hDecoder, pDispInfo->picture_index, &d_pSrcData,
        &nSrcPitch, &videoProcessingParameters));

    if ( !bgraData ) {
        size_t nSize = sizeof(unsigned char)*GetWidth()*GetHeight()*4;
        cuSafeCall(cudaMalloc((void**)&bgraData, nSize));
    }
    nv12_to_bgra(GetWidth(), GetHeight(), (unsigned char*)d_pSrcData, nSrcPitch, (unsigned char*)bgraData, GetWidth()*4);

    cuSafeCall(cuvidUnmapVideoFrame(hDecoder, d_pSrcData));
    return 1;
}

bool SNCuvidDecoder::Create(int _codec, int device)
{
    switch (_codec) {
        case NvCodec_MPEG1:
            eCodec = cudaVideoCodec_MPEG1;
            break;
        case NvCodec_MPEG2:
            eCodec = cudaVideoCodec_MPEG2;
            break;
        case NvCodec_MPEG4:
            eCodec = cudaVideoCodec_MPEG4;
            break;
        case NvCodec_H264:
            eCodec = cudaVideoCodec_H264;
            break;
        default:
            LOG_ERROR(logger, "Can't support this codec");
            return false;
    }
    cuSafeCall(cuInit(0));
    cuSafeCall(cuCtxCreate(&cuContext, 0, device));
    cuSafeCall(cuvidCtxLockCreate(&ctxLock, cuContext));

    CUVIDPARSERPARAMS videoParserParameters = {};
    videoParserParameters.CodecType = eCodec;
    videoParserParameters.ulMaxNumDecodeSurfaces = N_DECODE_SURFACE;
    videoParserParameters.ulMaxDisplayDelay = 1;
    videoParserParameters.pUserData = this;
    videoParserParameters.pfnSequenceCallback = HandleVideoSequenceProc;
    videoParserParameters.pfnDecodePicture = HandlePictureDecodeProc;
    videoParserParameters.pfnDisplayPicture = HandlePictureDisplayProc;
    if (pMutex) pMutex->lock();
    cuSafeCall(cuvidCreateVideoParser(&hParser, &videoParserParameters));
    if (pMutex) pMutex->unlock();
    return true;
}

bool SNCuvidDecoder::Release() {
    if (hParser) {
        cuSafeCall(cuvidDestroyVideoParser(hParser));
    }
    if (hDecoder) {
        if (pMutex) pMutex->lock();
        cuSafeCall(cuvidDestroyDecoder(hDecoder));
        if (pMutex) pMutex->unlock();
    }

    cuSafeCall(cuvidCtxLockDestroy(ctxLock));
    if (bgraData) {
        cuSafeCall(cudaFree(bgraData));
    }
    if (tempData) {
        cuSafeCall(cudaFree(tempData));
    }
    return true;
}

bool SNCuvidDecoder::Decode(const uint8_t *pData, int nSize, uint8_t *pFrame, int* outsize, int ofmt) {
    if (!hParser) {
        LOG_ERROR(logger, "Parser not initialized.");
        return false;
    }
    nDecodedFrame = 0;
    CUVIDSOURCEDATAPACKET packet = {0};
    packet.payload = pData;
    packet.payload_size = nSize;
    if (!pData || nSize == 0) {
        packet.flags = CUVID_PKT_ENDOFSTREAM;
    }
    if (pMutex) pMutex->lock();
    cuSafeCall(cuvidParseVideoData(hParser, &packet));
    if (pMutex) pMutex->unlock();

    if (nDecodedFrame > 0 && pFrame) {
        GetDecodedFrame(pFrame, outsize, ofmt);
    }

    return true;
}

bool SNCuvidDecoder::GetDecodedFrame(uint8_t* pFrame, int* pOutSize, int ofmt) {
    switch (ofmt) {
        case NvColor_BGR :
            *pOutSize = GetWidth()*GetHeight()*3;
            if (!tempData) {
                cuSafeCall(cudaMalloc((void**)&tempData, *pOutSize));
            }
            bgra_to_bgr(bgraData, tempData, GetHeight(), GetWidth());
            cuSafeCall(cudaMemcpy((unsigned char*)pFrame, tempData, *pOutSize, cudaMemcpyDefault));
            break;
        case NvColor_BGRA :
            *pOutSize = GetWidth()*GetHeight()*4;
            cuSafeCall(cudaMemcpy((unsigned char*)pFrame, bgraData, *pOutSize, cudaMemcpyDeviceToHost));
            break;
        case NvColor_YUV :
            bgra_to_yuv(bgraData, tempData, GetHeight(), GetWidth());
            *pOutSize = GetWidth()*GetHeight()*4;
            cuSafeCall(cudaMemcpy((unsigned char*)pFrame, tempData, *pOutSize, cudaMemcpyDeviceToHost));
            break;
        default:
            return false;
    }
    return true;
}

} // NAMESPACE SN_DECODER
