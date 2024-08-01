//***********************************************************
//  RTSP server for combined picture from multiple cameras
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************
#include <cstring>
#pragma warning(disable : 4996)
#include "CameraFrameSource.h"

   CameraFrameSource* CameraFrameSource::createNew(UsageEnvironment& env) 
   {
      return new CameraFrameSource(env);
   };

   CameraFrameSource::CameraFrameSource(UsageEnvironment& env) :
      FramedSource(env)
   {
      fEventTriggerId = envir().taskScheduler().createEventTrigger(CameraFrameSource::deliverFrameStub);
   };

  
   void CameraFrameSource::doStopGettingFrames()
   {
      FramedSource::doStopGettingFrames();
   };

   void CameraFrameSource::onFrame()
   {
      envir().taskScheduler().triggerEvent(fEventTriggerId, this);
   };

   void CameraFrameSource::doGetNextFrame()
   {
      deliverFrame();
   };

static uint8_t newFrameDataStart[1024 * 1024];
static unsigned newFrameSize = 0;

   void CameraFrameSource::deliverFrame()
   {
      printf("get!\n");
      if (!isCurrentlyAwaitingData()) return; // we're not ready for the buff yet

      printf("get! %d\n", fFrameSize);
      /* get the buff frame from the Encoding thread.. */
      // if (fEncoder->fetch_packet(&newFrameDataStart, &newFrameSize))
      if (1)
      {
         if (newFrameDataStart != nullptr) {
            /* This should never happen, but check anyway.. */
            if (newFrameSize > fMaxSize) {
               fFrameSize = fMaxSize;
               fNumTruncatedBytes = newFrameSize - fMaxSize;
               printf("over! %d\n", fMaxSize);
            }
            else {
               fFrameSize = newFrameSize;
            }

            gettimeofday(&fPresentationTime, nullptr);
            memcpy(fTo, newFrameDataStart, fFrameSize);

            //delete newFrameDataStart;
            //newFrameSize = 0;
         }
         else
         {
            fFrameSize = 0;
            fTo = nullptr;
            handleClosure(this);
         }
      }
      else {
         fFrameSize = 0;
      }

      if (fFrameSize > 0)
         FramedSource::afterGetting(this);
   };

int CameraFrameSource::put_frame(char *buf, int len)
{

   if (len < 1024*1024)
   {
      memcpy(newFrameDataStart, buf, len);
      fFrameSize = len;
      onFrame();
   }
}
