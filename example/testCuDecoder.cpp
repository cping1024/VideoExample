#include <stdio.h>
#include <unistd.h>
#include <chrono>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <sn_gpucodec_api.h>
#include <util/sn_video_frame_provider.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Args error!\n");
        return -1;
    }

    sn_codec_handle_t handle = sn_create_decoder(SN_CODEC_H264, 0);
    if (!handle) {
        return -1;
    }

    const char* url = argv[1];
    const int interval = 25;
    SNVideoFrameReader reader;
    int ret = reader.initReader(std::string(url));
    if (ret != 0) {
        return -1;
    }

    ret = reader.start();
    if (ret != 0) {
        reader.deInitReader();
        return -1;
    }

    cv::Mat img;    
    img.create(1080, 1920, CV_8UC3);
    const int out_fmt = SN_CH_FMT_BGR;

    int count = 0;
    long sum = 0;
    while(true) {
        void* buf = nullptr;
        int size = 0;

        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        ret = reader.readVideoFrame(&buf, &size);
        if (ret != 0) {
            continue;
        }

        int out_size = 0;
        int out_width = 0;
        int out_height = 0;
        int ret = sn_decode(handle, buf, size, img.data, &out_size, &out_width, &out_height, out_fmt);
        //printf("decode frame ret [%d], out size [%d].\n", ret, out_size);
        if (ret != 0 || out_size == 0) {
            continue;
        }

        std::chrono::steady_clock::time_point stop = std::chrono::steady_clock::now();
        std::chrono::milliseconds time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        sum += time.count();
        ++count;
        //printf("decode frame time [%d] ms.\n", time);
        cv::imshow("decode", img);
        if(static_cast<char>(cv::waitKey(/*interval*/40)) == 'q') break;
    }

    printf("decode test, count [%d],  avg time[%d]ms.\n", count, sum/count);

    sn_destroy_decoder(handle);
    /// stop provider
    reader.stop();
    printf ("cudecoder test!\n");
    return 0;
}
