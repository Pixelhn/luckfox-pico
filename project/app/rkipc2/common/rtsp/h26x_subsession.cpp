
#include "h26x_subsession.hh"
 
h26x_subsession::h26x_subsession(UsageEnvironment & env, FramedSource * source, int (*cb_func)(unsigned char*, unsigned int*)) : OnDemandServerMediaSubsession(env, True)
{
	m_pSource = source;
	m_pSDPLine = 0;
    cb_ReadFrame = cb_func;
}
 
h26x_subsession::~h26x_subsession(void)
{
	if (m_pSDPLine)
	{
		free(m_pSDPLine);
	}
}
 
h26x_subsession * h26x_subsession::createNew(UsageEnvironment & env, FramedSource * source, int (*cb_func)(unsigned char*, unsigned int*))
{
	return new h26x_subsession(env, source, cb_func);
}
 
FramedSource * h26x_subsession::createNewStreamSource(unsigned clientSessionId, unsigned & estBitrate)
{
	printf("[%s]", __func__);
	return H265VideoStreamFramer::createNew(envir(), new h26x_source(envir(), cb_ReadFrame));
}
 
RTPSink * h26x_subsession::createNewRTPSink(Groupsock * rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource * inputSource)
{
	printf("[%s]", __func__);
	return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
 
char const * h26x_subsession::getAuxSDPLine(RTPSink * rtpSink, FramedSource * inputSource)
{
	if (m_pSDPLine)
	{
		return m_pSDPLine;
	}
 
	m_pDummyRTPSink = rtpSink;
 
	//mp_dummy_rtpsink->startPlaying(*source, afterPlayingDummy, this);
	m_pDummyRTPSink->startPlaying(*inputSource, 0, 0);
 
	chkForAuxSDPLine(this);
 
	m_done = 0;
 
	envir().taskScheduler().doEventLoop(&m_done);
 
	m_pSDPLine = strdup(m_pDummyRTPSink->auxSDPLine());
 
	m_pDummyRTPSink->stopPlaying();
 
	return m_pSDPLine;
}
 
void h26x_subsession::afterPlayingDummy(void * ptr)
{
	h26x_subsession * This = (h26x_subsession *)ptr;
 
	This->m_done = 0xff;
}
 
void h26x_subsession::chkForAuxSDPLine(void * ptr)
{
	h26x_subsession * This = (h26x_subsession *)ptr;
 
	This->chkForAuxSDPLine1();
}
 
void h26x_subsession::chkForAuxSDPLine1()
{
	if (m_pDummyRTPSink->auxSDPLine())
	{
		m_done = 0xff;
	}
	else
	{
		double delay = 1000.0 / (FRAME_PER_SEC);  // ms
		int to_delay = delay * 1000;  // us
 
		nextTask() = envir().taskScheduler().scheduleDelayedTask(to_delay, chkForAuxSDPLine, this);
	}
}

