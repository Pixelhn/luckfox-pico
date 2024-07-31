#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "rtsp_live.h"

#define ACCESS_CONTROL

#ifdef __cplusplus
// Link with C way
extern "C" {
#endif

UsageEnvironment* env;
RTSPServer *rtspServer;
UserAuthenticationDatabase *authDB = NULL;

int rtsp_live_init()
{
    // Begin by setting up our usage environment:
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
    #ifdef SERVER_USE_TLS
    // Serve RTSPS: RTSP over a TLS connection:
    RTSPServer *rtspServer = RTSPServer::createNew(*env, 322, authDB);
    #else
    // Serve regular RTSP (over a TCP connection):
    rtspServer = RTSPServer::createNew(*env, 8554, authDB);
#endif
    if (rtspServer == NULL)
    {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
        return -1;
    }


    return 0;
}

#ifdef __cplusplus
}
#endif
