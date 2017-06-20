#ifndef __SN_GPUCODEC_H_
#define __SN_GPUCODEC_H_

#include <stdint.h>

#ifdef __cplusplus
    extern "C" {
#endif

/** @brief This API supports the following video stream formats */
enum _SN_CODEC_TYPE {
    SN_CODEC_MPEG1 = 0,
    SN_CODEC_MPEG2 = 1,
    SN_CODEC_MPEG4 = 2,
    SN_CODEC_H264  = 3
};

/** @brief This API supports the following output video stream chroma format */
enum _SN_CHROMFORMAT_{
    SN_CH_FMT_BGRA = 0,
    SN_CH_FMT_BGR  = 1,
    SN_CH_FMT_YUV  = 2,
    SN_CH_FMT_YUVA = 3
};

/**
 * Input picture structure
 */
typedef enum _SN_PIC_TYPE
{
    SN_PIC_STRUCT_FRAME             = 0x01,                 /**< Progressive frame */
    SN_PIC_STRUCT_FIELD_TOP_BOTTOM  = 0x02,                 /**< Field encoding top field first */
    SN_PIC_STRUCT_FIELD_BOTTOM_TOP  = 0x03                  /**< Field encoding bottom field first */
} SN_PIC_TYPE;

/**
 * Input picture type
 */
typedef enum _SN_FRAME_TYPE
{
    SN_PIC_TYPE_P               = 0x0,     /**< Forward predicted */
    SN_PIC_TYPE_B               = 0x01,    /**< Bi-directionally predicted picture */
    SN_PIC_TYPE_I               = 0x02,    /**< Intra predicted picture */
    SN_PIC_TYPE_IDR             = 0x03,    /**< IDR picture */
    SN_PIC_TYPE_BI              = 0x04,    /**< Bi-directionally predicted with only Intra MBs */
    SN_PIC_TYPE_SKIPPED         = 0x05,    /**< Picture is skipped */
    SN_PIC_TYPE_INTRA_REFRESH   = 0x06,    /**< First picture in intra refresh cycle */
    SN_PIC_TYPE_UNKNOWN         = 0xFF     /**< Picture type unknown */
} SN_FRAME_TYPE;

typedef struct _SN_ENCODE_FRAME
{
    uint32_t                frameIdx;                    /**< [out]: Frame no. for which the bitstream is being retrieved. */
    uint32_t                hwEncodeStatus;              /**< [out]: The NvEncodeAPI interface status for the locked picture. */
    uint32_t                numSlices;                   /**< [out]: Number of slices in the encoded picture. Will be reported only if NV_ENC_INITIALIZE_PARAMS::reportSliceOffsets set to 1. */
    uint32_t                bitstreamSizeInBytes;        /**< [out]: Actual number of bytes generated and copied to the memory pointed by bitstreamBufferPtr. */
    uint64_t                outputTimeStamp;             /**< [out]: Presentation timestamp associated with the encoded output. */
    uint64_t                outputDuration;              /**< [out]: Presentation duration associates with the encoded output. */
    void*                   bitstreamBufferPtr;          /**< [out]: Pointer to the generated output bitstream. */
    SN_FRAME_TYPE           pictureType;                 /**< [out]: Picture type of the encoded picture. */
    SN_PIC_TYPE             pictureStruct;               /**< [out]: Structure of the generated output picture. */
    uint32_t                frameAvgQP;                  /**< [out]: Average QP of the frame. */
    uint32_t                frameSatd;                   /**< [out]: Total SATD cost for whole frame. */
    uint32_t                ltrFrameIdx;                 /**< [out]: Frame index associated with this LTR frame. */
    uint32_t                ltrFrameBitmap;              /**< [out]: Bitmap of LTR frames indices which were used for encoding this frame. Value of 0 if no LTR frames were used. */
} SN_ENCODE_FRAME;

/// codec handle
typedef void* sn_codec_handle_t;

/// Codec Performance test
/// HardWare : GTX-970, i7-4790K
/// Decode: 500Frames/s
/// Encode: 125Frames/s

/************************Decoder*********************************/
/// @brief create decoder
/// @param[in] codec codec type
/// @param[in] cpu device id
/// @return if create succeed return decoder pointer, else return NULL
sn_codec_handle_t sn_create_decoder(int codec, int device);

/// @brief destroy decoder handle
int sn_destroy_decoder(sn_codec_handle_t handle);

/// @brief decode the video frame
/// @param[in] handle decoder handle
/// @param[in] in the input video frame
/// @param[in] insize video frame size
/// @param[out] out  decoded frame
/// @param[out] outsize  decoded frame size
/// @param[out] w decoded frame width
/// @param[out] h decoded frame height
/// @return if decode succeed return 0 , else return others
/// @notes there is no frame returned while outsize==0
int sn_decode(sn_codec_handle_t handle, void *in, int insize, void *out,  int *outsize, int *w, int *h, int ofmt);

/************************Encode*********************************/
/// @brief create decoder
/// @param[in] width encode frame width
/// @param[in] height encode frame height
/// @param[in] device GPU device id
/// @return if create succeed return encoder pointer, else return NULL
/// @note input frame fmt only support YUV420/YUV444
sn_codec_handle_t sn_create_encoder(int width, int height, int device);

/// @brief destroy decoder handle
int sn_destroy_encoder(sn_codec_handle_t handle);


/// @brief decode the video frame
/// @param[in] handle encoder handle
/// @param[in] frame the input video frame
/// @param[out] bitstream encoder frame bit stream, alloced by user
/// @param[out] streamsize frame bit stream size
/// @return if encode succeed return 0 , else return others
int sn_encode(sn_codec_handle_t handle, const uint8_t* frame, SN_ENCODE_FRAME* encoded_frame);

#ifdef __cplusplus
    }
#endif

#endif //__SN_GPUCODEC_H_
