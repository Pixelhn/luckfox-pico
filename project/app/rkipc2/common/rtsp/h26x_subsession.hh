#pragma once
 
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
 
#include "OnDemandServerMediaSubsession.hh"
#include "h26x_source.hh"
 
class h26x_subsession : public OnDemandServerMediaSubsession
{
public:
	h26x_subsession(UsageEnvironment & env, FramedSource * source, int (*cb_func)(unsigned char*, unsigned int*));
	~h26x_subsession(void);
 
public:
	virtual char const * getAuxSDPLine(RTPSink * rtpSink, FramedSource * inputSource);
	virtual FramedSource * createNewStreamSource(unsigned clientSessionId, unsigned & estBitrate); // "estBitrate" is the stream's estimated bitrate, in kbps
	virtual RTPSink * createNewRTPSink(Groupsock * rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource * inputSource);
 
	static h26x_subsession * createNew(UsageEnvironment & env, FramedSource * source, int (*cb_func)(unsigned char*, unsigned int*));
 
	static void afterPlayingDummy(void * ptr);
 
	static void chkForAuxSDPLine(void * ptr);
	
	void chkForAuxSDPLine1();
 
private:
	FramedSource * m_pSource;
	char * m_pSDPLine;
	RTPSink * m_pDummyRTPSink;
	char m_done;
	int (*cb_ReadFrame)(unsigned char *pbuff, unsigned int *len);
};

