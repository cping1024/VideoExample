#include <stdio.h>
#include <thread>
#include <memory>
#include <util/sn_video_packet_list.h>
#include <libavformat/avformat.h>
#include <chrono>

#define MAX_PACKET_LEN (1000)

void produce_packet(SNVideoPacketList* list) {
	for (int ix = 0; ix < MAX_PACKET_LEN; ++ix) {
		std::this_thread::sleep_for(std::chrono::milliseconds(40));
		AVPacket* pkt = av_packet_alloc();
		if (!pkt) {
			continue;
		} 
		
		pkt->duration = ix;		
		printf("produce packet [%d]!\n", ix);
		list->Push(pkt);
	}

	printf("producer exit!\n");
}


void consume_packet(SNVideoPacketList* list) {

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		AVPacket* pkt = list->Pop();
		if (!pkt) {
			printf("pop a null pkt!\n");	
			continue;
		}
		
		int duration = pkt->duration;
		av_packet_free(&pkt);
		printf("consume packet [%d]!\n", duration);
		if (duration == (MAX_PACKET_LEN - 1)) {
			break;
		}
	}

	printf("consumer exit!\n");
}

int main(int argc, char* argv[])
{
	//std::shared_ptr<SNVideoPacketList> list;
	//list.reset(new SNVideoPacketList(100));
	SNVideoPacketList* list = new SNVideoPacketList(100);
	if (!list) {
		return -1;
	}

	/// start produce thread
	std::thread producer(&produce_packet, list);
	/// start consume thread
	std::thread consumer(&consume_packet, list);
	
	if (producer.joinable()) {
		producer.join();	
	}

	if (consumer.joinable()) {
		consumer.join();
	}

	printf("VideoPacketList Test Exit!\n");
	return 0;
}
