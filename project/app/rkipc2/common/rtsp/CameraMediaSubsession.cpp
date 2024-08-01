//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************

#include "CameraMediaSubsession.h"

   CameraMediaSubsession* CameraMediaSubsession::createNew(UsageEnvironment& env, StreamReplicator* replicator)
   {
      return new CameraMediaSubsession(env, replicator);
   }

   FramedSource* CameraMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
   {

      printf("[%s]\n\n\n\n\n\n\n", __func__);
      estBitrate = fBitRate;
      FramedSource* source = m_replicator->createStreamReplica();
      return H265VideoStreamDiscreteFramer::createNew(envir(), source);
   }

   RTPSink* CameraMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
   {
      printf("[%s]\n\n\n\n\n\n\n", __func__);
      return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);//, fSPSNAL, fSPSNALSize, fPPSNAL, fPPSNALSize);
   }
