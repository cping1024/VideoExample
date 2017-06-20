#include <util/sn_video_packet_list.h>
#include <stdlib.h>
#include <iostream>

SNVideoPacketList::SNVideoPacketList(int max_len) 
:head_(nullptr), 
 tail_(nullptr),
 max_len_(max_len),
 count_(0)
{
    tail_ = head_ = (SN_AVPacketNode_t*) malloc(sizeof(SN_AVPacketNode_t));
	head_->pkt = nullptr;
	head_->next = nullptr;
}

SNVideoPacketList::~SNVideoPacketList()
{
    std::lock_guard<std::mutex> lk(mutex_);
    SN_AVPacketNode_t* node = head_;
    while(node != nullptr) {
        SN_AVPacketNode* cur = node;
        node = node->next;

        AVPacket* pkt = cur->pkt;
        if (pkt) {
            av_packet_free(&pkt);
        }

        delete cur;
    }
}

AVPacket* SNVideoPacketList::Pop()
{
    std::unique_lock<std::mutex> lk(mutex_);
    while (count_ == 0) {
        read_.wait(lk);
    }

    SN_AVPacketNode_t *next = head_->next;
    if(!next) {
        return nullptr;
    }

    head_->next = next->next;
    if (!head_->next) {
        tail_ = head_;
    }

    AVPacket *packet = next->pkt;
    delete next;

    --count_;
    write_.notify_one();
    return packet;
}

AVPacket* SNVideoPacketList::Try_Pop()
{
    std::lock_guard<std::mutex> lk(mutex_);
    if (count_ == 0) {
        return nullptr;
    }

    SN_AVPacketNode_t *next = head_->next;
    if(!next) {
        return nullptr;
    }

    head_->next = next->next;
    if (!head_->next) {
 	tail_ = head_;
    }
	    
    AVPacket *packet = next->pkt;
    delete next;

    --count_;
    write_.notify_one();
    return packet;
}

int SNVideoPacketList::Push(AVPacket* pkt)
{

    if(!head_ || !tail_) {
        return -1;
    }

    std::unique_lock<std::mutex> lk(mutex_);
    while(count_ >= max_len_) {
        write_.wait(lk);
    }

    SN_AVPacketNode_t *node = new SN_AVPacketNode_t;
    if(!node) {
        return -1;
    }

    node->pkt = pkt;
    node->next = NULL;

    tail_->next = node;
    tail_ = node;
    ++count_;
    read_.notify_one();	
    return 0;
}

int SNVideoPacketList::Try_Push(AVPacket *pkt)
{
    if(!head_ || !tail_) {
        return -1;
    }

    std::lock_guard<std::mutex> lk(mutex_);
    if (count_ >= max_len_) {
        return -1;
    }

    SN_AVPacketNode_t *node = new SN_AVPacketNode_t;
    if(!node) {
        return -1;
    }

    node->pkt = pkt;
    node->next = NULL;

    tail_->next = node;
    tail_ = node;
    ++count_;
    read_.notify_one();
    return 0;
}
