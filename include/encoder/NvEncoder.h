////////////////////////////////////////////////////////////////////////////
//
// Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
//
// Please refer to the NVIDIA end user license agreement (EULA) associated
// with this source code for terms and conditions that govern your use of
// this software. Any use, reproduction, disclosure, or distribution of
// this software and related documentation outside the terms of the EULA
// is strictly prohibited.
//
////////////////////////////////////////////////////////////////////////////

#include <encoder/NvHWEncoder.h>

#define MAX_ENCODE_QUEUE 32
#define FRAME_QUEUE 240

#define SET_VER(configStruct, type) {configStruct.version = type##_VER;}

template<class T>
class CNvQueue {
    T** m_pBuffer;
    unsigned int m_uSize;
    unsigned int m_uPendingCount;
    unsigned int m_uAvailableIdx;
    unsigned int m_uPendingndex;
public:
    CNvQueue(): m_pBuffer(NULL), m_uSize(0), m_uPendingCount(0), m_uAvailableIdx(0),
                m_uPendingndex(0)
    {
    }

    ~CNvQueue()
    {
        delete[] m_pBuffer;
    }

    bool Initialize(T *pItems, unsigned int uSize)
    {
        m_uSize = uSize;
        m_uPendingCount = 0;
        m_uAvailableIdx = 0;
        m_uPendingndex = 0;
        m_pBuffer = new T *[m_uSize];
        for (unsigned int i = 0; i < m_uSize; i++)
        {
            m_pBuffer[i] = &pItems[i];
        }
        return true;
    }


    T * GetAvailable()
    {
        T *pItem = NULL;
        if (m_uPendingCount == m_uSize)
        {
            return NULL;
        }
        pItem = m_pBuffer[m_uAvailableIdx];
        m_uAvailableIdx = (m_uAvailableIdx+1)%m_uSize;
        m_uPendingCount += 1;
        return pItem;
    }

    T* GetPending()
    {
        if (m_uPendingCount == 0) 
        {
            return NULL;
        }

        T *pItem = m_pBuffer[m_uPendingndex];
        m_uPendingndex = (m_uPendingndex+1)%m_uSize;
        m_uPendingCount -= 1;
        return pItem;
    }
};

typedef struct _EncodeFrameConfig
{
    uint8_t  *yuv[3];
    uint32_t stride[3];
    uint32_t width;
    uint32_t height;
}EncodeFrameConfig;

typedef struct ENCODE_BITSTREAM
{
    uint32_t                version;                     /**< [in]: Struct version. Must be set to ::NV_ENC_LOCK_BITSTREAM_VER. */
    uint32_t                doNotWait         :1;        /**< [in]: If this flag is set, the NvEncodeAPI interface will return buffer pointer even if operation is not completed. If not set, the call will block until operation completes. */
    uint32_t                ltrFrame          :1;        /**< [out]: Flag indicating this frame is marked as LTR frame */
    uint32_t                reservedBitFields :30;       /**< [in]: Reserved bit fields and must be set to 0 */
    void*                   outputBitstream;             /**< [in]: Pointer to the bitstream buffer being locked. */
    uint32_t*               sliceOffsets;                /**< [in,out]: Array which receives the slice offsets. This is not supported if NV_ENC_CONFIG_H264::sliceMode is 1 on Kepler GPUs. Array size must be equal to size of frame in MBs. */
    uint32_t                frameIdx;                    /**< [out]: Frame no. for which the bitstream is being retrieved. */
    uint32_t                hwEncodeStatus;              /**< [out]: The NvEncodeAPI interface status for the locked picture. */
    uint32_t                numSlices;                   /**< [out]: Number of slices in the encoded picture. Will be reported only if NV_ENC_INITIALIZE_PARAMS::reportSliceOffsets set to 1. */
    uint32_t                bitstreamSizeInBytes;        /**< [out]: Actual number of bytes generated and copied to the memory pointed by bitstreamBufferPtr. */
    uint64_t                outputTimeStamp;             /**< [out]: Presentation timestamp associated with the encoded output. */
    uint64_t                outputDuration;              /**< [out]: Presentation duration associates with the encoded output. */
    void*                   bitstreamBufferPtr;          /**< [out]: Pointer to the generated output bitstream. */
    NV_ENC_PIC_TYPE         pictureType;                 /**< [out]: Picture type of the encoded picture. */
    NV_ENC_PIC_STRUCT       pictureStruct;               /**< [out]: Structure of the generated output picture. */
    uint32_t                frameAvgQP;                  /**< [out]: Average QP of the frame. */
    uint32_t                frameSatd;                   /**< [out]: Total SATD cost for whole frame. */
    uint32_t                ltrFrameIdx;                 /**< [out]: Frame index associated with this LTR frame. */
    uint32_t                ltrFrameBitmap;              /**< [out]: Bitmap of LTR frames indices which were used for encoding this frame. Value of 0 if no LTR frames were used. */
    uint32_t                reserved [236];              /**< [in]: Reserved and must be set to 0 */
    void*                   reserved2[64];               /**< [in]: Reserved and must be set to NULL */
} ENCODE_BITSTREAM;

typedef enum 
{
    NV_ENC_DX9 = 0,
    NV_ENC_DX11 = 1,
    NV_ENC_CUDA = 2,
    NV_ENC_DX10 = 3,
} NvEncodeDeviceType;

class CNvEncoder
{
public:
    CNvEncoder();
    virtual ~CNvEncoder();

    int Initialize(int width, int height, int device);
    int Encode(const uint8_t* frame, NV_ENC_LOCK_BITSTREAM* bitstream);
    int Deinitialize();

protected:
    CNvHWEncoder                                        *m_pNvHWEncoder = NULL;
    uint32_t                                             m_uEncodeBufferCount = 0;
    uint32_t                                             m_uPicStruct = 0;
    EncodeConfig encodeConfig;

    int lumaPlaneSize = 0;
    int chromaPlaneSize = 0;
    int numFramesEncoded = 0;

    void*                                                m_pDevice = NULL;
    uint8_t *yuv[3] = {NULL,NULL,NULL};

    CUcontext                                            m_cuContext;
    EncodeConfig                                         m_stEncoderInput;
    EncodeBuffer                                         m_stEncodeBuffer[MAX_ENCODE_QUEUE];
    CNvQueue<EncodeBuffer>                               m_EncodeBufferQueue;
    EncodeOutputBuffer                                   m_stEOSOutputBfr; 

private:
    int GetEncodedBitStream(NV_ENC_LOCK_BITSTREAM* bitstream);

private:
    NVENCSTATUS                                          EncodeFrame(EncodeFrameConfig *pEncodeFrame, bool bFlush=false, uint32_t width=0, uint32_t height=0);
    NVENCSTATUS                                          InitCuda(uint32_t deviceID = 0);
    NVENCSTATUS                                          AllocateIOBuffers(uint32_t uInputWidth, uint32_t uInputHeight, uint32_t isYuv444);
    NVENCSTATUS                                          ReleaseIOBuffers();
    unsigned char*                                       LockInputBuffer(void * hInputSurface, uint32_t *pLockedPitch);
    NVENCSTATUS                                          FlushEncoder();
    NVENCSTATUS                                          RunMotionEstimationOnly(MEOnlyConfig *pMEOnly, bool bFlush);
};

// NVEncodeAPI entry point
typedef NVENCSTATUS (NVENCAPI *MYPROC)(NV_ENCODE_API_FUNCTION_LIST*); 
