#ifndef _BIT_STREAM_BUFFER_SOURCE_HH
#define _BIT_STREAM_BUFFER_SOURCE_HH

#include <liveMedia/FramedSource.hh>
#include <util/Framebuffer.h>
#include <util/SN_Frame.h>

#include <memory>

class BitStreamBufferSource: public FramedSource {
public:
  static BitStreamBufferSource* createNew(UsageEnvironment& env,
                     unsigned frameBufferSize,
					 unsigned preferredFrameSize = 0,
					 unsigned playTimePerFrame = 0);
  // "preferredFrameSize" == 0 means 'no preference'
  // "playTimePerFrame" is in microseconds

  void addFrame(const std::shared_ptr<SNFrame>& frame);

  unsigned maxFrameSize() const {return 1024 * 1024;}
protected:
  BitStreamBufferSource(UsageEnvironment& env,
               unsigned frameBufferSiz,
		       unsigned preferredFrameSize,
		       unsigned playTimePerFrame);
	// called only by createNew()

  virtual ~BitStreamBufferSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

protected:

private:
  FrameBuffer<std::shared_ptr<SNFrame> >* buffer_;

  unsigned fPreferredFrameSize;
  unsigned fPlayTimePerFrame;
  unsigned fLastPlayTime;
};

#endif // _BIT_STREAM_BUFFER_SOURCE_HH
