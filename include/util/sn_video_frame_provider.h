#ifndef __SN_VIDEO_FRAME_READER_
#define __SN_VIDEO_FRAME_READER_

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

#include <string>
#include <atomic>
#include <memory>
#include <util/sn_video_packet_list.h>
class SNVideoFrameReader
{
public:
    SNVideoFrameReader();
    ~SNVideoFrameReader();
	
    /// init frame provider
    int initReader(const std::string& url);

    void deInitReader();

	int start();

	void stop();

	int readVideoFrame(void** framebuf, int* buf_len, bool blocked = true);
	
private:
    void read();

private:
    bool init_ = false;
    bool running_ = false;
    int video_index_ = -1;

    std::atomic_bool exit_;

    AVPacket* pkt_ = NULL;
    AVFormatContext* avformatctx_ = NULL;
    std::string url_;
    std::shared_ptr<SNVideoPacketList> list_;
};

#endif // __SN_VIDEO_FRAME_READER_
