#include <stdio.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "liveMedia/liveMedia.hh"
#include "BasicUsageEnviroment/BasicUsageEnvironment.hh"

#include <encoder/NvEncoder.h>
#include <sn_gpucodec_api.h>
#include <transcoder/H264BitStreamServerMediaSubsession.h>
#include <transcoder/BitStreamBufferSource.h>

#include <util/SN_Frame.h>
#include <thread>

UsageEnvironment* env;

// To make the second and subsequent client for each stream reuse the same
// input stream as the first client (rather than playing the file from the
// start for each client), change the following "False" to "True":
Boolean reuseFirstSource = False;

// To stream *only* MPEG-1 or 2 video "I" frames
// (e.g., to reduce network bandwidth),
// change the following "False" to "True":
Boolean iFramesOnly = False;

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
               char const* streamName, char const* inputFileName){
    char* url = rtspServer->rtspURL(sms);
    UsageEnvironment& env = rtspServer->envir();
    env << "\n\"" << streamName << "\" stream, from the file \""
        << inputFileName << "\"\n";
    env << "Play this stream using the URL \"" << url << "\"\n";
    delete[] url;
}

void threadFun(void* args, void* args1)
{
    H264BitStreamServerMediaSubsession* subSession = static_cast<H264BitStreamServerMediaSubsession*>(args1);
    if (!subSession) {
        return;
    }
    /// init video capture
    std::string filename = std::string((char*)args);
    cv::VideoCapture capture(filename);
    if (!capture.isOpened()) {
        return;
    }

    int device = 0;
    /// init video encoder
    sn_codec_handle_t encoder = sn_create_encoder(1920, 1080, device);
    if (!encoder) {
        return;
    }

    /// init decoder
    sn_codec_handle_t decoder = sn_create_decoder(SN_CODEC_H264, device);
    if (!decoder) {
        sn_destroy_encoder(encoder);
        return;
    }


    cv::Mat img;
    img.create(1080, 1920, CV_8UC3);
    SN_ENCODE_FRAME encoded_frame;
    encoded_frame.bitstreamBufferPtr = NULL;

    int count = 0;
    cv::Mat frame;
    int FirstIFrame = 0;
    std::shared_ptr<SNFrame> IDRframe;
    while (capture.read(frame)) {

        ++count;

        cv::Mat image_yuv420;
        cv::cvtColor(frame, image_yuv420, CV_BGR2YUV_I420);
        if (!encoded_frame.bitstreamBufferPtr) {
            encoded_frame.bitstreamBufferPtr = new uint8_t[frame.cols * frame.rows * frame.channels()];
        }

        int ret = sn_encode(encoder, (uint8_t*)image_yuv420.data, &encoded_frame);
        if (ret != 0 || encoded_frame.bitstreamSizeInBytes == 0) {
            continue;
        }

        /// save IDR Frame
        if (count == 1) {
            IDRframe.reset(new SNFrame((const uint8_t*)encoded_frame.bitstreamBufferPtr,\
                                       encoded_frame.bitstreamSizeInBytes));
            printf("IDR Frame size [%d].\n", encoded_frame.bitstreamSizeInBytes);
        }

        /// add bitstream to RTSPServer
        BitStreamBufferSource* frameSource = subSession->GetSource();
        if (frameSource) {
            std::shared_ptr<SNFrame> addframe;
            addframe.reset(new SNFrame((const uint8_t*)encoded_frame.bitstreamBufferPtr, encoded_frame.bitstreamSizeInBytes));
            /// force First frame is IDR Frame
            if (FirstIFrame > 2) {
                frameSource->addFrame(addframe);
            } else {
                /// some player may can't recv IDR at first time, so send 2 times
                if (encoded_frame.pictureType == SN_PIC_TYPE_IDR) {
                    frameSource->addFrame(IDRframe);
                    FirstIFrame++;
                }
            }
        }


        /// if need to display raw video ,cancel comment

        int out_size = 0;
        int width = 0;
        int height = 0;
        ret = sn_decode(decoder, encoded_frame.bitstreamBufferPtr, encoded_frame.bitstreamSizeInBytes,\
                        img.data, &out_size, &width, &height, SN_CH_FMT_BGR);
        if (ret == 0 && out_size != 0) {
            cv::imshow("decode", img);
        }

        if (static_cast<char>(cv::waitKey(25)) == 'q') {
            break;
        }
    }

    if (encoded_frame.bitstreamBufferPtr) {
        delete encoded_frame.bitstreamBufferPtr;
    }

    sn_destroy_encoder(encoder);
    sn_destroy_decoder(decoder);
}

int main(int argc, char* argv[]) 
{
    if (argc < 2) {
        printf("Args error.\n");
        return -1;
    }

    /// init mediaserver enviroment
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);
    /// init server authentication module
    UserAuthenticationDatabase * authDB = NULL;
    //new UserAuthenticationDatabase;
    //authDB->addUserRecord("admin", "admin");
    /// create rtsp server
    RTSPServer* rtspServer = RTSPServer::createNew(*env, 8552, authDB);
    if (!rtspServer) {
        return -1;
    }

    const char* descriptionStr = "Session streamed by \"testRTPSMediaServer:q\"";
    const char* streamName = "H264BitStreamTest";
    ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, NULL, descriptionStr);
    H264BitStreamServerMediaSubsession* subSession = \
            H264BitStreamServerMediaSubsession::createNew(*env, reuseFirstSource);
    sms->addSubsession(subSession);
    rtspServer->addServerMediaSession(sms);
    announceStream(rtspServer, sms, streamName, NULL);

    /// start codec thread
    std::thread thread(threadFun, (void*)argv[1], subSession);
    if (thread.joinable()) {
        thread.detach();
    }

    if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080)) {
      //*env << "\n(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling.)\n";
        printf("We use port [%d] for optional RTSP-over-HTTP tunneling.\n", rtspServer->httpServerPortNum());
    } else {
      //*env << "\n(RTSP-over-HTTP tunneling is not available.)\n";
        printf("RTSP-over-HTTP tunneling is not available.\n");
    }

    env->taskScheduler().doEventLoop(); // does not return
    return 0;
}
