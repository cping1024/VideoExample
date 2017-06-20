// "liveMedia"
// Copyright (c) 1996-2017 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a H264 video file.
// Implementation

#include <transcoder/H264BitStreamServerMediaSubsession.h>
#include <liveMedia/H264VideoRTPSink.hh>
#include <liveMedia/H264VideoStreamFramer.hh>

H264BitStreamServerMediaSubsession*
H264BitStreamServerMediaSubsession::createNew(UsageEnvironment& env,
					      Boolean reuseFirstSource) {
  return new H264BitStreamServerMediaSubsession(env, reuseFirstSource);
}

H264BitStreamServerMediaSubsession::H264BitStreamServerMediaSubsession(
        UsageEnvironment& env, Boolean reuseFirstSource)
  : OnDemandServerMediaSubsession(env, reuseFirstSource){
}

H264BitStreamServerMediaSubsession::~H264BitStreamServerMediaSubsession() {
}


FramedSource* H264BitStreamServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 500; // kbps, estimate

  // Create the video source:
  /*BitStreamBufferSource**/
  Source_ = BitStreamBufferSource::createNew(envir(), 25);
  if (!Source_) return NULL;

  // Create a framer for the Video Elementary Stream:
  return H264VideoStreamFramer::createNew(envir(), Source_);
}

RTPSink* H264BitStreamServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* /*inputSource*/) {
  return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
