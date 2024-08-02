#pragma once
 
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
 
#include "OnDemandServerMediaSubsession.hh"
#include "h26x_source.hh"
 
class h26x_subsession : public OnDemandServerMediaSubsession
{
public:
	h26x_subsession(UsageEnvironment & env, int (*cb_func)(unsigned char*, unsigned int*));
	~h26x_subsession(void);
 
public:
	static h26x_subsession * createNew(UsageEnvironment & env, int (*cb_func)(unsigned char*, unsigned int*));

	virtual FramedSource * createNewStreamSource(unsigned clientSessionId, unsigned & estBitrate); // "estBitrate" is the stream's estimated bitrate, in kbps
	virtual RTPSink * createNewRTPSink(Groupsock * rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource * inputSource);
	virtual char const * getAuxSDPLine(RTPSink * rtpSink, FramedSource * inputSource);
 
private:
	// FramedSource * m_pSource;
	char * m_pSDPLine;
	RTPSink * m_pDummyRTPSink;
	// char m_done;
	int (*cb_ReadFrame)(unsigned char *pbuff, unsigned int *len);
};

