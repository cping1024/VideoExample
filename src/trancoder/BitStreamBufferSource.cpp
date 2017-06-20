#include <transcoder/BitStreamBufferSource.h>
#include <liveMedia/InputFile.hh>
#include <groupsock/GroupsockHelper.hh>

////////// BitStreamBufferSource //////////

BitStreamBufferSource*
BitStreamBufferSource::createNew(UsageEnvironment& env, unsigned frameBufferSize,
				unsigned preferredFrameSize,
				unsigned playTimePerFrame) {
  BitStreamBufferSource* newSource
    = new BitStreamBufferSource(env, frameBufferSize, preferredFrameSize, playTimePerFrame);
  return newSource;
}

void BitStreamBufferSource::addFrame(const std::shared_ptr<SNFrame> &frame)
{
    if (buffer_) {
        buffer_->Push(frame);
    }
}

BitStreamBufferSource::BitStreamBufferSource(UsageEnvironment& env, unsigned frameBufferSize,
					   unsigned preferredFrameSize,
					   unsigned playTimePerFrame)
  : FramedSource(env), fPreferredFrameSize(preferredFrameSize),
    fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0){

    buffer_ = new FrameBuffer<std::shared_ptr<SNFrame> >(frameBufferSize);
}

BitStreamBufferSource::~BitStreamBufferSource() {
    if (buffer_) {
        buffer_->NoMoreJobs();
        delete buffer_;
    }
}

void BitStreamBufferSource::doGetNextFrame() {
    if (!buffer_) {
        return;
    }

    std::shared_ptr<SNFrame> frame;
    bool ret = buffer_->Pop(&frame);
    if (!ret) {
        return;
    }

    fFrameSize = frame->Size();
    memcpy(fTo, frame->GetData(), fFrameSize);
    //printf("memcpy frame to source , frame size[%d].\n", fFrameSize);

    // Set the 'presentation time':
    if (fPlayTimePerFrame > 0 && fPreferredFrameSize > 0) {
      if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
        // This is the first frame, so use the current time:
        gettimeofday(&fPresentationTime, NULL);
      } else {
        // Increment by the play time of the previous data:
        unsigned uSeconds = fPresentationTime.tv_usec + fLastPlayTime;
        fPresentationTime.tv_sec += uSeconds/1000000;
        fPresentationTime.tv_usec = uSeconds%1000000;
      }

      // Remember the play time of this data:
      fLastPlayTime = (fPlayTimePerFrame*fFrameSize)/fPreferredFrameSize;
      fDurationInMicroseconds = fLastPlayTime;
    } else {
      // We don't know a specific play time duration for this data,
      // so just record the current time as being the 'presentation time':
      gettimeofday(&fPresentationTime, NULL);
    }

    if (fFrameSize > fMaxSize)
    {
        fNumTruncatedBytes = fFrameSize - fMaxSize;
        fFrameSize = fMaxSize;
    }
    else
    {
        fNumTruncatedBytes = 0;
    }

    // Inform the reader that he has data:
    // Because the file read was done from the event loop, we can call the
    // 'after getting' function directly, without risk of infinite recursion:
    //FramedSource::afterGetting(this);
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0,(TaskFunc*)FramedSource::afterGetting, this);
}

