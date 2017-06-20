#ifndef __SN_FRAME_H_
#define __SN_FRAME_H_

#include <stdint.h>
#include <string.h>

class SNFrame
{
public:
    SNFrame(const uint8_t* data, int size):size_(size) {
        data_ = new uint8_t[size_];
        memcpy(data_, data, size_);
    }

    ~SNFrame() {
        if (data_) {
            delete data_;
            data_ = NULL;
        }
    }
    const uint8_t* GetData(){ return data_;}
    unsigned int Size(){ return size_;}

    SNFrame(const SNFrame&) = delete;
    SNFrame& operator=(const SNFrame&) = delete;

private:
    unsigned int size_;
    uint8_t* data_;
};
#endif //__SN_FRAME_H_
