#include <util/sn_video_frame_provider.h>
#include <thread>
#include <unistd.h>
#include <chrono>

SNVideoFrameReader::SNVideoFrameReader()
{
    exit_= false;
    list_.reset(new SNVideoPacketList(75));
}

SNVideoFrameReader::~SNVideoFrameReader()
{
    /// exit reader thread	
    if (list_->empty()) {
        list_->Push((AVPacket*) NULL);
    } else if (list_->full()) {
        AVPacket* pkt = list_->Pop();
    } else {
        exit_ = true;
    }

    /// make sure exit thread
    std::this_thread::sleep_for(std::chrono::milliseconds(40));

    if (init_) {
        deInitReader();
    }

    if (pkt_) {
        av_packet_free(&pkt_);
    	pkt_ = nullptr;
    }
}

int SNVideoFrameReader::initReader(const std::string& url)
{
    if (init_) {
        return 0;
    }

    url_ =  url;
    av_register_all();

    int ret = avformat_network_init();
    if (ret != 0) {
        return ret;
    }

    avformatctx_ = avformat_alloc_context();
    if (avformatctx_ == NULL) {
        avformat_network_deinit();
        return ret;
    }

    ret = avformat_open_input(&avformatctx_, url_.c_str(), NULL, NULL);
    if (ret != 0) {
        avformat_free_context(avformatctx_);
        avformat_network_deinit();
        return ret;
    }

    ret = avformat_find_stream_info(avformatctx_, NULL);
    if (ret < 0) {
        avformat_free_context(avformatctx_);
        avformat_network_deinit();
        return ret;
    }

    for (int ix = 0; ix < avformatctx_->nb_streams; ++ix) {
        if (avformatctx_->streams[ix]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index_ = ix;
            break;
        }
    }

    if (video_index_ == -1) {
        avformat_free_context(avformatctx_);
        avformat_network_deinit();
        return -1;
    }

    init_ = true;
    return 0;
}

void SNVideoFrameReader::deInitReader()
{
    if (avformatctx_) {
        avformat_free_context(avformatctx_);
    }

    avformat_network_deinit();
    init_ = false;
}

int SNVideoFrameReader::start()
{
    if (!init_) {
        return -1;
    }

    if (running_) {
        return 0;
    }

    std::thread thread(&SNVideoFrameReader::read, this);
    if (thread.joinable()) {
        thread.detach();
    }

    running_ = true;
	return 0;
}

void SNVideoFrameReader::stop()
{
    exit_ = true;
}

int SNVideoFrameReader::readVideoFrame(void** framebuf, int* buf_len, bool blocked)
{
    if (!list_) {
        return -1;
    }
	
    if (pkt_) {
        av_packet_free(&pkt_);
        pkt_ = nullptr;
    }	

    if (blocked) {
        pkt_ = list_->Pop();
    } else {
        pkt_ = list_->Try_Pop();
    }

    if (pkt_) {
        *framebuf = (void*)pkt_->data;
        *buf_len = pkt_->size;
    } else {
        return -1;
    }	

    return 0;
}

void SNVideoFrameReader::read()
{
    while(!exit_) {
        AVPacket *packet = av_packet_alloc();
        if (!packet) {
            continue;
        }

        //std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        int ret = av_read_frame(avformatctx_, packet);
        if (ret == 0 && packet->stream_index == video_index_) {
            list_->Push(packet);
        }

        //std::chrono::steady_clock::time_point stop = std::chrono::steady_clock::now();
        //std::chrono::milliseconds time_used = \
        //        std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        //long time_sleep = (25 - time_used.count()) > 0 ? 25 - time_used.count():0;
        //std::this_thread::sleep_for(std::chrono::milliseconds(time_sleep));
    }

    running_ = false;
}
