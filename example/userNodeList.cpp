#include <stdio.h>
#include <mutex>
#include <atomic>
#include <iostream>
#include <condition_variable>
#include <chrono>
#include <thread>

#define MAX_PACKET_LEN (200)

typedef struct UserNode {
    struct UserNode* next;
    int id;
} UserNode_t;

class UserNodeList {
public:
    UserNodeList(int max_len):max_len_(max_len) {
        head_ = tail_ = new UserNode_t;
        head_->id = 0;
        head_->next = NULL;
    }

    ~UserNodeList() {
        UserNode_t* node = head_;
        while (node) {
            UserNode_t* next = node->next;
            delete node;
            node = next;
        }
    }

    UserNode_t* Pop() {
        if (!head_)  {
            return nullptr;
        }

        std::unique_lock<std::mutex> lk(mutex_);
        while (count_ == 0) {
            read_.wait(lk);
        }

        UserNode_t* node = head_->next;
        if (!node) {
            return nullptr;
        }

        head_->next = node->next;
        if (!head_->next) {
            tail_ = head_;
        }
        --count_;
        std::cout << "pop a node ,id:" << node->id << ", count:" << count_ << std::endl;
        write_.notify_one();
        return node;
    }

    int Push(UserNode_t* node) {
        if (!tail_) {
            return -1;
        }

        std::unique_lock<std::mutex> lk(mutex_);
        while (count_ >= max_len_) {
            write_.wait(lk);
        }

        tail_->next = node;
        tail_ = node;
        ++count_;
        std::cout << "push a node ,id:" << node->id << ", count:" << count_ << std::endl;
        read_.notify_one();
        return 0;
    }

private:
    UserNode_t* head_;
    UserNode_t* tail_;
    int max_len_;
    std::atomic_uint count_;
    std::mutex	mutex_;
    std::condition_variable read_;
    std::condition_variable write_;
};

void produce(UserNodeList* list) {
    for (int ix = 1; ix <= MAX_PACKET_LEN; ++ix) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        UserNode_t* node = new UserNode;
        if (!node) {
            continue;
        }

        node->id = ix;
        list->Push(node);
    }

    printf("producer exit!\n");
}


void consume(UserNodeList* list) {

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        UserNode_t* node = list->Pop();
        if (!node) {
            std::cout << "pop null node!" << std::endl;
            continue;
        }

        int id = node->id;
        delete node;
        if (id == (MAX_PACKET_LEN)) {
            break;
        }
    }

    printf("consumer exit!\n");
}

UserNodeList* list = new UserNodeList(100);

int main() {

    /// start produce thread
    std::thread producer(produce, list);
    /// start consume thread
    std::thread consumer(consume, list);

    if (producer.joinable()) {
        producer.join();
    }

    if (consumer.joinable()) {
        consumer.join();
    }

	return 0;
}
