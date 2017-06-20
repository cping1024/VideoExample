#ifndef _H264_BIT_STREAM_SERVER_MEDIA_SUBSESSION_H_
#define _H264_BIT_STREAM_SERVER_MEDIA_SUBSESSION_H_

#include "liveMedia/OnDemandServerMediaSubsession.hh"
#include <transcoder/BitStreamBufferSource.h>

class H264BitStreamServerMediaSubsession: public OnDemandServerMediaSubsession {
public:
  static H264BitStreamServerMediaSubsession*
  createNew(UsageEnvironment& env, Boolean reuseFirstSource);

  virtual ~H264BitStreamServerMediaSubsession();

  BitStreamBufferSource* GetSource() {return Source_;}

protected:
  H264BitStreamServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource);
      // called only by createNew();

protected: // redefined virtual functions
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);

private:
  BitStreamBufferSource* Source_ = NULL;
};

#endif // _H264_BIT_STREAM_SERVER_MEDIA_SUBSESSION_H_
