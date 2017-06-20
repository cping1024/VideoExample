#ifndef __BITSTREAM_SOURCE_MODULE_H_
#define __BITSTREAM_SOURCE_MODULE_H_

#include <memory>
#include <mutex>

#include <util/SN_Frame.h>
#include <transcoder/BitStreamBufferSource.h>

class BitStreamSourceModule
{
public:
    /// not MultiThread safe
    static BitStreamSourceModule* GetInstance();
    ~BitStreamSourceModule();

    BitStreamSourceModule& operator=(const BitStreamSourceModule&) = delete;
    BitStreamSourceModule (const BitStreamSourceModule&) = delete;

    void SaveIDRFrame(std::shared_ptr<SNFrame>& frame);

    void Push(const std::shared_ptr<SNFrame>& frame);

    void AddSource(BitStreamBufferSource* source);

    bool empty();
private:
    BitStreamSourceModule();

private:
    std::shared_ptr<SNFrame> IDRFrame_;
    std::mutex mutex_;
    std::vector<BitStreamBufferSource*> source_vec_;
};
#endif //__BITSTREAM_SOURCE_MODULE_H_
