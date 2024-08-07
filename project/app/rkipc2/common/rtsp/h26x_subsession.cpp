#include "h26x_subsession.hh"
 
h26x_subsession::h26x_subsession(UsageEnvironment & env, int (*cb_func)(unsigned char*, unsigned int*, struct timeval *)) : OnDemandServerMediaSubsession(env, True)
{
	// m_pSource = source;
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
 
h26x_subsession * h26x_subsession::createNew(UsageEnvironment & env, int (*cb_func)(unsigned char*, unsigned int*, struct timeval *))
{
	return new h26x_subsession(env, cb_func);
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
	printf("{%s}1\n\n\n\n\n\n\n", __func__);
	return m_pSDPLine;
}

