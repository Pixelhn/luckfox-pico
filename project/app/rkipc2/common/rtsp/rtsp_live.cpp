

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "h26x_subsession.hh"
#include "frame_module.h"
// #include "video_parse.h"
#include "rtsp_live.h"
#include <cstdio>
#include <cstring>
#include <pthread.h>

UsageEnvironment* env;
FrameQueue g_stFrmQ;

int cb_readframe(unsigned char *pbuff, unsigned int *len) 
{
    printf("%s \n", __func__);

    HI_VFRAME vf;
    if(frame_queue_used(&g_stFrmQ)<= 0 )
    {
        *len = 0;
    }
    else
    {
        frame_queue_pop(&g_stFrmQ, &vf);
        memcpy(pbuff, vf.vbuff, vf.vlen);
        *len = vf.vlen; 
    }

    return 0;
}

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms, char const* streamName, char const* inputFileName)
{
  char* url = rtspServer->rtspURL(sms);
  UsageEnvironment& env = rtspServer->envir();
  env << "\n\"" << streamName << "\" stream, from the file \""
      << inputFileName << "\"\n";
  env << "Play this stream using the URL \"" << url << "\"\n";
  delete[] url;
}

int rtsp_live_init()
{
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
  // To implement client access control to the RTSP server, do the following:
  authDB = new UserAuthenticationDatabase;
  authDB->addUserRecord("username1", "password1"); // replace these with real strings
  // Repeat the above with each <username>, <password> that you wish to allow
  // access to the server.
#endif

  // Create the RTSP server:
  RTSPServer* rtspServer = RTSPServer::createNew(*env, 8554, authDB);
  if (rtspServer == NULL)
  {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }

  char const* descriptionString
    = "Session streamed by \"testOnDemandRTSPServer\"";
  {
    char const* streamName = "h264ESVideoTest";
    char const* inputFileName = "test.264";
   	h26x_source * H264LSrc;
   
    frame_queue_init(&g_stFrmQ, 20);
    
    ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, streamName, descriptionString);

    OutPacketBuffer::maxSize = 1024 * 512;

    sms->addSubsession(h26x_subsession::createNew(*env, H264LSrc, cb_readframe));
    rtspServer->addServerMediaSession(sms);

    announceStream(rtspServer, sms, streamName, inputFileName);
  }

    printf("\n\n\n\n\n\n\n\n\n");
  return 0; // only to prevent compiler warning
}

int rtsp_create_chaneal_session(char *dir)
{
    return 0;
}

int rtsp_start()
{

    env->taskScheduler().doEventLoop(); // does not return

    return 0;
}

int rtsp_put(char *buf, int len)
{
    HI_VFRAME vf;
        if(frame_queue_remain(&g_stFrmQ) <= 0) {
            printf("WARN: frame full, reset it.\n");
            frame_queue_reset(&g_stFrmQ);
        }
        memcpy(vf.vbuff, buf, len);
        vf.vlen = len;
        frame_queue_push(&g_stFrmQ, &vf);
    return 0;
}
