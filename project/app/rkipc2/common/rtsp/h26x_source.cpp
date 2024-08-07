
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "h26x_source.hh"
#include "rtsp_live.h"
 
 
h26x_source::h26x_source(UsageEnvironment & env, int (*cb_func)(unsigned char*, unsigned int*, struct timeval *)) : 
    FramedSource(env), m_pToken(0), cb_ReadFrame(cb_func)
{
	if(cb_func == NULL) 
    {
		printf("[MEDIA SERVER] call back func is NULL, failed\n");
		return;
    }

	// m_eventTriggerId = envir().taskScheduler().createEventTrigger(h26x_source::getNextFrame);
	rtsp_begin();
}
 
h26x_source::~h26x_source(void)
{	
	// envir().taskScheduler().unscheduleDelayedTask(m_pToken);
	printf("[MEDIA SERVER] rtsp connection closed\n");
	rtsp_stop();
}
 
void h26x_source::doGetNextFrame()
{
	printf("[%s]", __func__);
	this->GetFrameData();
}
 
unsigned int h26x_source::maxFrameSize() const
{
	return 1024*512;
}
 
void h26x_source::getNextFrame(void * ptr)
{
	printf("[%s]", __func__);
	((h26x_source *)ptr)->GetFrameData();
}
 
void h26x_source::GetFrameData()
{
	printf("[%s]", __func__);

    // fill frame data
    if(cb_ReadFrame)
	{
        cb_ReadFrame(fTo, &fFrameSize, &fPresentationTime);
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
				 
	afterGetting(this);
}


