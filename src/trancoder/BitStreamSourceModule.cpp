#include <transcoder/BitStreamSourceModule.h>


BitStreamSourceModule *BitStreamSourceModule::GetInstance()
{
    static BitStreamSourceModule module;
    return &module;
}

BitStreamSourceModule::~BitStreamSourceModule()
{

}

void BitStreamSourceModule::Push(const std::shared_ptr<SNFrame> &frame)
{
    std::lock_guard<std::mutex> lk(mutex_);
    int len = source_vec_.size();
    for (int ix = 0; ix < len; ++ix) {
        BitStreamBufferSource* source = source_vec_[ix];
        if (!source) {
            continue;
        }

        source->addFrame(frame);
    }
}

void BitStreamSourceModule::AddSource(BitStreamBufferSource *source)
{
    std::lock_guard<std::mutex> lk(mutex_);
    bool exist = false;
    int len = source_vec_.size();
    for (int ix = 0; ix < len; ++ix) {
        BitStreamBufferSource* framesource = source_vec_[ix];
        if (source == framesource ) {
            exist = true;
            break;
        }
    }

    if (!exist) {
        /// add IDR Frame
        source->addFrame(IDRFrame_);

        /// in case some player can't recv IDR Frame, send again
        source->addFrame(IDRFrame_);
        source_vec_.push_back(source);

        printf("Module add frame source.\n");
    }
}

bool BitStreamSourceModule::empty()
{
    std::lock_guard<std::mutex> lk(mutex_);
    return source_vec_.empty();
}

BitStreamSourceModule::BitStreamSourceModule()
{

}

void BitStreamSourceModule::SaveIDRFrame(std::shared_ptr<SNFrame> &frame)
{
    IDRFrame_ = frame;
}
