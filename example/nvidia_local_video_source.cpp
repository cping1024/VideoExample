#include <stdio.h>
#include <cuviddec.h>
#include <nvcuvid.h>
#include <string.h>

int NVIDIASOURCECALLBACK(void* userdata, CUVIDSOURCEDATAPACKET *packet) 
{
	printf("read A packet.\n");
	printf("Packet size [%d].\n", packet->payload_size);
}

int main(int argc, char* argv[]) 
{
	if (argc < 2) {
		printf("Args error!\n");
		return -1;
	}	
	
	const char* video_file = argv[1];
	printf("video file path [%s] \n", video_file);	
	CUvideosource videosource;
	CUVIDSOURCEPARAMS param;
	param.ulClockRate = 10 * 1000 * 1000;
	memset(param.uReserved1, 0, sizeof(param.uReserved1));
	param.pUserData = nullptr;
	param.pfnVideoDataHandler = NVIDIASOURCECALLBACK;
	param.pfnAudioDataHandler = nullptr;
	memset(param.pvReserved2, NULL, sizeof(param.pvReserved2));

	CUresult ret = cuvidCreateVideoSource(&videosource, video_file, &param);
	/// nvidia display driver version is later
	if (ret != CUDA_SUCCESS) {
		printf("create video source fail, ret:[%d]\n", ret);
		return ret;
	}

	cudaVideoState state = cuvidGetVideoSourceState(videosource);
	printf("video source state [%d] \n", state);

	ret = cuvidDestroyVideoSource(videosource);
	if (ret != CUDA_SUCCESS) {
		printf("destroy video source fail, ret:[%d]\n", ret);
		return ret;
	}
	
	return 0;
}
