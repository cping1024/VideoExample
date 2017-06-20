#ifndef __CODEC_COLORSPACE_H_
#define __CODEC_COLORSPACE_H_


void nv12_to_bgra(int width, int height, unsigned char *dpNv12, int nNv12Pitch, unsigned char *dpRgb, int nRgbPitch);

void bgra_to_bgr(const unsigned char* in, unsigned char* out, int height, int width);

void bgra_to_yuv(const unsigned char* in, unsigned char* out, int height, int width);

#endif //__CODEC_COLORSPACE_H_
