#include <stdio.h>
#include <string>
#include <chrono>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <encoder/NvEncoder.h>
#include <sn_gpucodec_api.h>

using namespace std::chrono;

int testNvEncoder(const std::string& filename){
    cv::VideoCapture capture(filename);
    if (!capture.isOpened()) {
        return -1;
    }
    const int device_id = 0;
    /// init video encoder
    sn_codec_handle_t encoder = sn_create_encoder(1920, 1080, device_id);
    if (!encoder) {
        return -1;
    }
    /// init decoder
    sn_codec_handle_t decoder = sn_create_decoder(SN_CODEC_H264, device_id);
    if (!decoder) {
        return -1;
    }

    cv::Mat img;
    img.create(1080, 1920, CV_8UC3);
    SN_ENCODE_FRAME encoded_frame;
    encoded_frame.bitstreamBufferPtr = NULL;

    int count = 0;
    long sum = 0;
    const int interval(25);
    cv::Mat frame;
    while (capture.read(frame)) {

        cv::Rect rect(10, 10, 100, 100);
        cv::rectangle(frame, rect, cv::Scalar(0, 255, 0), 2);
        cv::putText(frame, std::string("encode frame:") + std::to_string(count),
                    cv::Point(500, 500),
                    cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(0, 0, 255), 2);
        if(!encoded_frame.bitstreamBufferPtr) {
            encoded_frame.bitstreamBufferPtr = new uint8_t[frame.cols * frame.rows * frame.channels()];
        }

        cv::Mat image_yuv420;
        cv::cvtColor(frame, image_yuv420, CV_BGR2YUV_I420);

        steady_clock::time_point start = steady_clock::now();
        int ret = sn_encode(encoder, (uint8_t*)image_yuv420.data, &encoded_frame);
        if (ret != 0 || encoded_frame.bitstreamSizeInBytes == 0) {
            ///decode a frame
            continue;
        }

        steady_clock::time_point stop = steady_clock::now();
        milliseconds time = duration_cast<milliseconds>(stop - start);
        sum += time.count();
        ++count;
        //printf("Encode Frame [%d] ms.\n", time);

        /*
        int out_size = 0;
        int width = 0;
        int height = 0;
        ret = sn_decode(decoder, encoded_frame.bitstreamBufferPtr, \
                        encoded_frame.bitstreamSizeInBytes, img.data, &out_size, &width, &height, SN_CH_FMT_BGR);
        if (ret != 0 || out_size == 0) {
            continue;
        }

        steady_clock::time_point stop1 = steady_clock::now();
        time = duration_cast<milliseconds>(stop1 - start);
        */
        cv::imshow("decode", frame);

        if (static_cast<char>(cv::waitKey(interval)) == 'q') {
            break;
        }
    }

    printf("encode test, count [%d],  avg time[%d]ms.\n", count, sum/count);

    if (encoded_frame.bitstreamBufferPtr) {
        delete encoded_frame.bitstreamBufferPtr;
    }

    sn_destroy_encoder(encoder);
    sn_destroy_decoder(decoder);
    return 0;
}

int main(int argc, char* argv[]) 
{
    if (argc < 2) {
        printf("Args error.\n");
        return -1;
    }

    std::string filename = std::string(argv[1]);
    testNvEncoder(filename);
    return 0;
}
