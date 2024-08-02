/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2024, Live Networks, Inc.  All rights reserved
// A test program that demonstrates how to stream - via unicast RTP
// - various kinds of file on demand, using a built-in RTSP server.
// main program

#include "liveMedia.hh"

#include "BasicUsageEnvironment.hh"
#include "announceURL.hh"

UsageEnvironment* env;

// To make the second and subsequent client for each stream reuse the same
// input stream as the first client (rather than playing the file from the
// start for each client), change the following "False" to "True":
Boolean reuseFirstSource = False;

// To stream *only* MPEG-1 or 2 video "I" frames
// (e.g., to reduce network bandwidth),
// change the following "False" to "True":
Boolean iFramesOnly = False;

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName, char const* inputFileName); // forward

static char newDemuxWatchVariable;

static MatroskaFileServerDemux* matroskaDemux;
static void onMatroskaDemuxCreation(MatroskaFileServerDemux* newDemux, void* /*clientData*/) {
  matroskaDemux = newDemux;
  newDemuxWatchVariable = 1;
}

static OggFileServerDemux* oggDemux;
static void onOggDemuxCreation(OggFileServerDemux* newDemux, void* /*clientData*/) {
  oggDemux = newDemux;
  newDemuxWatchVariable = 1;
}

int main(int argc, char** argv) {
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
#ifdef SERVER_USE_TLS
  // Serve RTSPS: RTSP over a TLS connection:
  RTSPServer* rtspServer = RTSPServer::createNew(*env, 322, authDB);
#else
  // Serve regular RTSP (over a TCP connection):
  RTSPServer* rtspServer = RTSPServer::createNew(*env, 8554, authDB);
#endif
  if (rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }
#ifdef SERVER_USE_TLS
#ifndef STREAM_USING_SRTP
#define STREAM_USING_SRTP True
#endif
  rtspServer->setTLSState(PATHNAME_TO_CERTIFICATE_FILE, PATHNAME_TO_PRIVATE_KEY_FILE,
			  STREAM_USING_SRTP);
#endif

  char const* descriptionString
    = "Session streamed by \"testOnDemandRTSPServer\"";

  // Set up each of the possible streams that can be served by the
  // RTSP server.  Each such stream is implemented using a
  // "ServerMediaSession" object, plus one or more
  // "ServerMediaSubsession" objects for each audio/video substream.

  // A H.265 video elementary stream:
  {
    char const* streamName = "h265ESVideoTest";
    char const* inputFileName = "test.265";
    ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, streamName, descriptionString);

    OutPacketBuffer::maxSize = 300000;
    sms->addSubsession(H265VideoFileServerMediaSubsession::createNew(*env, inputFileName, reuseFirstSource));


    char const* inputFileNamea = "test.opus";
    newDemuxWatchVariable = 0;
    ServerMediaSubsession* smss;
    OggFileServerDemux::createNew(*env, inputFileNamea, onOggDemuxCreation, NULL);
    env->taskScheduler().doEventLoop(&newDemuxWatchVariable);
    while ((smss = oggDemux->newServerMediaSubsession()) != NULL)
    {
      sms->addSubsession(smss);
    }

    rtspServer->addServerMediaSession(sms);

    announceStream(rtspServer, sms, streamName, inputFileName);
  }

  // An Opus ('.opus') audio file:
  // (Note: ".opus' files are special types of Ogg files, so we use the same code as the Ogg ('.ogg') file code above.)
  // {
  //   char const* streamName = "opusFileTest";
  //   char const* inputFileName = "test.opus";
  //   ServerMediaSession* sms
  //     = ServerMediaSession::createNew(*env, streamName, streamName,
	// 			      descriptionString);

  //   newDemuxWatchVariable = 0;
  //   OggFileServerDemux::createNew(*env, inputFileName, onOggDemuxCreation, NULL);
  //   env->taskScheduler().doEventLoop(&newDemuxWatchVariable);

  //   Boolean sessionHasTracks = False;
  //   ServerMediaSubsession* smss;
  //   while ((smss = oggDemux->newServerMediaSubsession()) != NULL) {
  //     sms->addSubsession(smss);
  //     sessionHasTracks = True;
  //   }
  //   if (sessionHasTracks) {
  //     rtspServer->addServerMediaSession(sms);
  //   }
  //   // otherwise, because the stream has no tracks, we don't add a ServerMediaSession to the server.

  //   announceStream(rtspServer, sms, streamName, inputFileName);
  // }


  // Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
  // Try first with the default HTTP port (80), and then with the alternative HTTP
  // port numbers (8000 and 8080).

// #ifdef SERVER_USE_TLS
//   // (Attempt to) use the default HTTPS port (443) instead:
//   char const* httpProtocolStr = "HTTPS";
//   if (rtspServer->setUpTunnelingOverHTTP(443)) {
// #else
//   char const* httpProtocolStr = "HTTP";
//   if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080)) {
// #endif
//     *env << "\n(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-" << httpProtocolStr << " tunneling.)\n";
//   } else {
//     *env << "\n(RTSP-over-" << httpProtocolStr << " tunneling is not available.)\n";
//   }

  env->taskScheduler().doEventLoop(); // does not return

  return 0; // only to prevent compiler warning
}

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName, char const* inputFileName) {
  UsageEnvironment& env = rtspServer->envir();

  env << "\n\"" << streamName << "\" stream, from the file \""
      << inputFileName << "\"\n";
  announceURL(rtspServer, sms);
}
