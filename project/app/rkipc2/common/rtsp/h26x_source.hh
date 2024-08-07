
#ifndef __H264LiveSource_H__
#define __H264LiveSource_H__
 
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "FramedSource.hh"
 
#define FRAME_PER_SEC 60
 
class h26x_source : public FramedSource
{
public:
	h26x_source(UsageEnvironment & env, int (*cb_func)(unsigned char*, unsigned int*, struct timeval *));
	~h26x_source(void);
 
public:
	virtual void doGetNextFrame();
	virtual unsigned int maxFrameSize() const;
 
	static void getNextFrame(void * ptr);
	void GetFrameData();
 
private:
	void *m_pToken;
	int (*cb_ReadFrame)(unsigned char *pbuff, unsigned int *len, struct timeval *fPresentationTime);
	EventTriggerId m_eventTriggerId;
};
 
#endif
