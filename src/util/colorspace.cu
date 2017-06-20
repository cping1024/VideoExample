/*
 * NV12ToARGB color space conversion CUDA kernel
 *
 * This sample uses CUDA to perform a simple NV12 (YUV 4:2:0 planar)
 * source and converts to output in ARGB format
 */
//#ifdef HAVE_NVCUVID

#include <util/colorspace.h>
#include <stdio.h>

static int divUp(int m, int n) {
	return (m+n-1) / n;
}

__constant__ float constHueColorSpaceMat[9] = { 1.1644f, 0.0f,1.596f, 1.1644f, -0.3918f, -0.813f, 1.1644f, 2.0172f, 0.0f };
__device__ static void YUV2RGB(const uint* yuvi, float* red, float* green, float* blue)
{
	float luma, chromaCb, chromaCr;
    // Prepare for hue adjustment
	luma     = (float)yuvi[0];
	chromaCb = (float)((int)yuvi[1] - 512.0f);
	chromaCr = (float)((int)yuvi[2] - 512.0f);
    // Convert YUV To RGB with hue adjustment
	*red   = (luma     * constHueColorSpaceMat[0]) +
		(chromaCb * constHueColorSpaceMat[1]) +
		(chromaCr * constHueColorSpaceMat[2]);
       
	*green = (luma     * constHueColorSpaceMat[3]) +
		(chromaCb * constHueColorSpaceMat[4]) +
		(chromaCr * constHueColorSpaceMat[5]);

    *blue  = (luma     * constHueColorSpaceMat[6]) +
		(chromaCb * constHueColorSpaceMat[7]) +
		(chromaCr * constHueColorSpaceMat[8]);
}

__device__ static uint RGBA_pack_10bit(float red, float green, float blue, uint alpha)
{
	uint ARGBpixel = 0;
	// Clamp final 10 bit results
	red   = ::fmin(::fmax(red,   0.0f), 1023.f);
	green = ::fmin(::fmax(green, 0.0f), 1023.f);
	blue  = ::fmin(::fmax(blue,  0.0f), 1023.f);
	// Convert to 8 bit unsigned integers per color component
	ARGBpixel = (((uint)blue  >> 2) |
			(((uint)green >> 2) << 8)  |
			(((uint)red   >> 2) << 16) |
			(uint)alpha);
	return ARGBpixel;
}

// CUDA kernel for outputing the final BGRA output from NV12
#define COLOR_COMPONENT_BIT_SIZE 10
#define COLOR_COMPONENT_MASK     0x3FF
__global__ void nv12_to_bgra_kernel(const unsigned char* in, size_t inpitch, uint* out, size_t outpitch, uint width, uint height)
{
	// Pad borders with duplicate pixels, and we multiply by 2 because we process 2 pixels per thread
	const int x = blockIdx.x * (blockDim.x << 1) + (threadIdx.x << 1);
	const int y = blockIdx.y *  blockDim.y       +  threadIdx.y;
	if (x >= width || y >= height)
		return;
	// Read 2 Luma components at a time, so we don't waste processing since CbCr are decimated this way.
	// if we move to texture we could read 4 luminance values
	uint yuv101010Pel[2];
	yuv101010Pel[0] = (in[y * inpitch + x    ]) << 2;
	yuv101010Pel[1] = (in[y * inpitch + x + 1]) << 2;
	const size_t chromaOffset = inpitch * height;
	const int y_chroma = y >> 1;
        
	if (y & 1) {  // odd scanline ?
		uint chromaCb = in[chromaOffset + y_chroma * inpitch + x    ];
		uint chromaCr = in[chromaOffset + y_chroma * inpitch + x + 1];
		if (y_chroma < ((height >> 1) - 1)) {// interpolate chroma vertically
			chromaCb = (chromaCb + in[chromaOffset + (y_chroma + 1) * inpitch + x    ] + 1) >> 1;
			chromaCr = (chromaCr + in[chromaOffset + (y_chroma + 1) * inpitch + x + 1] + 1) >> 1;
        }
        yuv101010Pel[0] |= (chromaCb << ( COLOR_COMPONENT_BIT_SIZE       + 2));
        yuv101010Pel[0] |= (chromaCr << ((COLOR_COMPONENT_BIT_SIZE << 1) + 2));
        yuv101010Pel[1] |= (chromaCb << ( COLOR_COMPONENT_BIT_SIZE       + 2));
        yuv101010Pel[1] |= (chromaCr << ((COLOR_COMPONENT_BIT_SIZE << 1) + 2));
    } else {
		yuv101010Pel[0] |= ((uint)in[chromaOffset + y_chroma * inpitch + x    ] << ( COLOR_COMPONENT_BIT_SIZE       + 2));
		yuv101010Pel[0] |= ((uint)in[chromaOffset + y_chroma * inpitch + x + 1] << ((COLOR_COMPONENT_BIT_SIZE << 1) + 2));
		yuv101010Pel[1] |= ((uint)in[chromaOffset + y_chroma * inpitch + x    ] << ( COLOR_COMPONENT_BIT_SIZE       + 2));
		yuv101010Pel[1] |= ((uint)in[chromaOffset + y_chroma * inpitch + x + 1] << ((COLOR_COMPONENT_BIT_SIZE << 1) + 2));
	}
	// this steps performs the color conversion
	uint yuvi[6];
	float red[2], green[2], blue[2];
	yuvi[0] =  (yuv101010Pel[0] &   COLOR_COMPONENT_MASK    );
	yuvi[1] = ((yuv101010Pel[0] >>  COLOR_COMPONENT_BIT_SIZE)       & COLOR_COMPONENT_MASK);
	yuvi[2] = ((yuv101010Pel[0] >> (COLOR_COMPONENT_BIT_SIZE << 1)) & COLOR_COMPONENT_MASK);
	yuvi[3] =  (yuv101010Pel[1] &   COLOR_COMPONENT_MASK    );
	yuvi[4] = ((yuv101010Pel[1] >>  COLOR_COMPONENT_BIT_SIZE)       & COLOR_COMPONENT_MASK);
	yuvi[5] = ((yuv101010Pel[1] >> (COLOR_COMPONENT_BIT_SIZE << 1)) & COLOR_COMPONENT_MASK);
	// YUV to RGB Transformation conversion
	YUV2RGB(&yuvi[0], &red[0], &green[0], &blue[0]);
	YUV2RGB(&yuvi[3], &red[1], &green[1], &blue[1]);
	// Clamp the results to RGBA
	const size_t pitch = outpitch >> 2;
	out[y * pitch + x     ] = RGBA_pack_10bit(red[0], green[0], blue[0], ((uint)0xff << 24));
	out[y * pitch + x + 1 ] = RGBA_pack_10bit(red[1], green[1], blue[1], ((uint)0xff << 24));
    
}

void nv12_to_bgra(int width, int height, unsigned char* in, int inpitch, unsigned char* out, int outpitch)
{
    // Final Stage: NV12toARGB color space conversion
    dim3 block(32, 8);
    dim3 grid(divUp(width, 2 * block.x), divUp(height, block.y));
    nv12_to_bgra_kernel<<<grid, block>>>(in, inpitch, (unsigned int*)out, outpitch, width, height);
}


__global__ void bgra_to_yuv_kernel(const unsigned char* in, unsigned char* out, int height, int width)
{
}
void bgra_to_yuv(const unsigned char* in, unsigned char* out, int height, int width) 
{
}

__global__ void bgra_to_bgr_kernel(const unsigned char* in, unsigned char* out, int height, int width)
{
	int tidx = blockIdx.x * blockDim.x + threadIdx.x;
	int tidy = blockIdx.y * blockDim.y + threadIdx.y;
	if (tidx < width && tidy < height) {
		out[tidy*width*3 + tidx*3 + 0] = in[tidy*width*4 + tidx*4 + 0];
		out[tidy*width*3 + tidx*3 + 1] = in[tidy*width*4 + tidx*4 + 1];
		out[tidy*width*3 + tidx*3 + 2] = in[tidy*width*4 + tidx*4 + 2];
	}
}

void bgra_to_bgr(const unsigned char* in, unsigned char* out, int height, int width)
{
	dim3 block(16, 16);
	dim3 grid(divUp(width, block.x), divUp(height, block.y));
	bgra_to_bgr_kernel<<<grid, block>>>(in, out, height, width);
}

//#endif
