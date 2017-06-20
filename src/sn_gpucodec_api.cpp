#include <sn_gpucodec_api.h>
#include <decoder/sn_cuvid_decoder.h>
#include <encoder/NvEncoder.h>

using namespace SN_DECODER;

sn_codec_handle_t sn_create_decoder(int codec, int device)
{
    SNCuvidDecoder* handle = new SNCuvidDecoder();
    if (NULL == handle) return NULL;
    if (!handle->Create(codec, device)) return NULL;
    return handle;
}

int sn_destroy_decoder(sn_codec_handle_t handle)
{
    SNCuvidDecoder *hdl = static_cast<SNCuvidDecoder*>(handle);
    if (!hdl) { return -1; }
    if (!hdl-> Release()) return -1;
    return 0;
}

int sn_decode(sn_codec_handle_t handle, void *in, int insize, void *out, int *outsize, int *w, int *h, int ofmt)
{
    *outsize = 0; *w = 0; *h = 0;
    SNCuvidDecoder *hdl = static_cast<SNCuvidDecoder*>(handle);
    if (!hdl) { return -1; }
    if (!hdl->Decode((uint8_t*)in, insize, (uint8_t*)out, outsize, ofmt)) { return -1; }
    if (*outsize > 0) {
        *w = hdl->GetWidth();
        *h = hdl->GetHeight();
    }
    return 0;
}

sn_codec_handle_t sn_create_encoder(int width, int height, int device)
{
    CNvEncoder* encoder = new CNvEncoder();
    if (!encoder) {
        return NULL;
    }

    int ret = encoder->Initialize(width, height, device);
    if (ret != 0) {
        delete encoder;
        return NULL;
    }

    return encoder;
}

/// @brief destroy decoder handle
int sn_destroy_encoder(sn_codec_handle_t handle)
{
    CNvEncoder *encoder = static_cast<CNvEncoder*>(handle);
    if (!encoder) {
        return -1;
    }

    if (encoder->Deinitialize() != 0) {
        return -1;
    }

    return 0;
}

int sn_encode(sn_codec_handle_t handle, const uint8_t* frame, SN_ENCODE_FRAME* encoded_frame)
{
    CNvEncoder *encoder = static_cast<CNvEncoder*>(handle);
    if (!encoder) {
        return -1;
    }

    NV_ENC_LOCK_BITSTREAM bitstream;
    bitstream.bitstreamBufferPtr = encoded_frame->bitstreamBufferPtr;
    int ret = encoder->Encode(frame, &bitstream);
    if (ret != 0) {
        return -1;
    }

    encoded_frame->frameIdx = bitstream.frameIdx;
    encoded_frame->hwEncodeStatus = bitstream.hwEncodeStatus;
    encoded_frame->numSlices = bitstream.numSlices;
    encoded_frame->bitstreamSizeInBytes = bitstream.bitstreamSizeInBytes;
    encoded_frame->outputTimeStamp = bitstream.outputTimeStamp;
    encoded_frame->outputDuration = bitstream.outputDuration;
    encoded_frame->bitstreamBufferPtr = bitstream.bitstreamBufferPtr;
    encoded_frame->pictureType = (SN_FRAME_TYPE)bitstream.pictureType;
    encoded_frame->pictureStruct = (SN_PIC_TYPE)bitstream.pictureStruct;
    encoded_frame->frameAvgQP = bitstream.frameAvgQP;
    encoded_frame->frameSatd = bitstream.frameSatd;
    encoded_frame->ltrFrameIdx = bitstream.ltrFrameIdx;
    encoded_frame->ltrFrameBitmap = bitstream.ltrFrameBitmap;
    return 0;
}

