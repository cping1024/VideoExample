#ifndef __SN_VIDEO_PACKET_LIST_H_
#define __SN_VIDEO_PACKET_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif 

#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

typedef struct SN_AVPacketNode
{
	AVPacket *pkt;
	struct SN_AVPacketNode* next;
} SN_AVPacketNode_t;

class SNVideoPacketList
{
public:
	SNVideoPacketList(int max_len);
	~SNVideoPacketList();

	AVPacket* Pop();
	AVPacket* Try_Pop();		
	
    int Push(AVPacket* pkt);
    int Try_Push(AVPacket* pkt);
	
    bool empty() { std::lock_guard<std::mutex> lk(mutex_); return count_ == 0 ? true : false;}
    bool full() { std::lock_guard<std::mutex> lk(mutex_); return count_ == max_len_ ? true : false;}
private:
	SN_AVPacketNode_t* head_;
	SN_AVPacketNode_t* tail_;
	int max_len_;
	std::atomic_uint count_;
	std::mutex	mutex_;
    std::condition_variable read_;
	std::condition_variable write_;
};
#endif //__SN_VIDEO_PACKET_LIST_H_
