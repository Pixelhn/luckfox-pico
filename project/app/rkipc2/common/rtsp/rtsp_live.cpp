#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "rtsp_live.h"
#include <GroupsockHelper.hh>
#include "CameraFrameSource.h"
#include "CameraMediaSubsession.h"

// #define ACCESS_CONTROL

#ifdef __cplusplus
// Link with C way
extern "C" {
#endif

UsageEnvironment* env;
RTSPServer *rtspServer;
UserAuthenticationDatabase *authDB = NULL;
pthread_t livre_thread_t;

void *livre_thread(void * agrv)
{
    env->taskScheduler().doEventLoop(); // does not return
    return 0;
}


void announceURL(RTSPServer* rtspServer, ServerMediaSession* sms)
{
    if (rtspServer == NULL || sms == NULL) return; // sanity check

    UsageEnvironment& env = rtspServer->envir();

    env << "Play this stream using the URL ";
    if (weHaveAnIPv4Address(env))
    {
        char* url = rtspServer->ipv4rtspURL(sms);
        env << "\"" << url << "\"";
        delete[] url;
        if (weHaveAnIPv6Address(env)) env << " or ";
    }
    if (weHaveAnIPv6Address(env))
    {
        char* url = rtspServer->ipv6rtspURL(sms);
        env << "\"" << url << "\"";
        delete[] url;
    }
    env << "\n";

    return;
}

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms, char const* streamName, char const* inputFileName)
{
  UsageEnvironment& env = rtspServer->envir();

    env << "\n\"" << streamName << "\" stream, from the file \""  << inputFileName << "\"\n";
    announceURL(rtspServer, sms);

    return;
}

int rtsp_live_init()
{
    Boolean reuseFirstSource = False;

    printf("[%s] init start!\n\n\n\n\n", __func__);

    TaskScheduler *scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

#ifdef ACCESS_CONTROL
    // To implement client access control to the RTSP server, do the following:
    authDB = new UserAuthenticationDatabase;
    authDB->addUserRecord("username1",
                            "password1"); // replace these with real strings
    // Repeat the above with each <username>, <password> that you wish to allow
    // access to the server.
#endif

    // Create the RTSP server:
    rtspServer = RTSPServer::createNew(*env, 8554, authDB);
    if (rtspServer == NULL)
    {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
        return -1;
    }

    // char const* descriptionString = "Session streamed by \"testOnDemandRTSPServer\"";
    // char const* streamName = "h265ESVideoTest";
    // char const* inputFileName = "/mnt/nfs/test.265";
    // ServerMediaSession* rtsp_main = ServerMediaSession::createNew(*env, streamName, streamName, descriptionString);

    // OutPacketBuffer::maxSize = 300000;
    // rtsp_main->addSubsession(H265VideoFileServerMediaSubsession::createNew(*env, inputFileName, reuseFirstSource));

    // rtspServer->addServerMediaSession(rtsp_main);
    // announceStream(rtspServer, rtsp_main, streamName, inputFileName);


    printf("[%s] init finish\n\n\n\n\n", __func__);
    return 0;
}

 CameraFrameSource* source;

int rtsp_create_chaneal_session(char *dir)
{

    ServerMediaSession* sms = NULL;
    sms = ServerMediaSession::createNew(*env, "hello");


    source = CameraFrameSource::createNew(*env);
    StreamReplicator *videoReplicator  = StreamReplicator::createNew(*env, source, false);;

    ServerMediaSubsession *sub = CameraMediaSubsession::createNew(*env, videoReplicator);;


    sms->addSubsession(sub);
    rtspServer->addServerMediaSession(sms);

    return 0;
}

int rtsp_start()
{
    pthread_create(&livre_thread_t, NULL, livre_thread, NULL);

    return 0;
}


int rtsp_put()
{
    printf("put %p\n", source);
    if (source)
        source->onFrame();
    return 0;
}

#ifdef __cplusplus
}
#endif
