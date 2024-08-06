

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "h26x_subsession.hh"
#include "h26x_source.hh"
#include "rtsp_live.h"
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h> 

UsageEnvironment* env;
RTSPServer* rtspServer;

pthread_mutex_t g_frame_buf_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_frame_buf_cond = PTHREAD_COND_INITIALIZER;
char g_buf[300 * 1024];
unsigned int g_len;
int g_frame_bufstatus = 0;
int fps;
int total;
struct timeval last;

int cb_readframe(unsigned char *pbuff, unsigned int *len) 
{
    struct timeval tv;
    static int fps_count = 0;
    static int total_size = 0;

    pthread_mutex_lock(&g_frame_buf_mutex);
    g_frame_bufstatus = 1;
    gettimeofday(&tv, NULL);
    printf("wait!  %ld-%ld\n", tv.tv_sec, tv.tv_usec);

    pthread_cond_wait(&g_frame_buf_cond, &g_frame_buf_mutex);   
    gettimeofday(&tv, NULL);
    printf("get!  %ld-%ld    fps %d  %d-%d\n\n\n\n\n\n\n", tv.tv_sec, tv.tv_usec, fps, total, total/1024);

    memcpy(pbuff, g_buf, g_len);
    *len = g_len;

    pthread_mutex_unlock(&g_frame_buf_mutex);
    
    if (tv.tv_sec != last.tv_sec)
    {
        last = tv;
        fps = fps_count;
        total = total_size;
        fps_count = 0;
        total_size  =0;
    }

    fps_count++;
    total_size += g_len;

    return 0;
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
    rtspServer = RTSPServer::createNew(*env, 8554, authDB);
    if (rtspServer == NULL)
    {
      *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
      exit(1);
    }

    printf("\n\n\n\n\n\n\n\n\n");
    return 0; // only to prevent compiler warning
}

int rtsp_create_chaneal_session(char *dir)
{
  char const* descriptionString = "Session streamed by \"testOnDemandRTSPServer\"";

    char const* streamName = "video";

   	// h26x_source * H264LSrc = new h26x_source(env, cb_ReadFrame);
    
    ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, streamName, descriptionString);
    OutPacketBuffer::maxSize = 1024 * 512;
    sms->addSubsession(h26x_subsession::createNew(*env, cb_readframe));
    rtspServer->addServerMediaSession(sms);

    return 0;
}

void *rtsp_thread(void *argv)
{

    env->taskScheduler().doEventLoop(); // does not return

    return 0;
}


int rtsp_start()
{

    pthread_t rtsp_thread_t;

    pthread_create(&rtsp_thread_t, NULL, rtsp_thread, NULL);

    return 0;
}

int rtsp_put(char *buf, int len, int type)
{
    struct timeval tv;
    pthread_mutex_lock(&g_frame_buf_mutex);
    do
    {
        if (g_frame_bufstatus == 0)
            break;

        memcpy(g_buf, buf, len);
        g_len = len;

        gettimeofday(&tv, NULL);
        printf("send  %ld-%ld\n", tv.tv_sec, tv.tv_usec);

        pthread_cond_broadcast(&g_frame_buf_cond);
        g_frame_bufstatus = 0;

    }
    while (0);
    pthread_mutex_unlock(&g_frame_buf_mutex);

    return 0;
}
