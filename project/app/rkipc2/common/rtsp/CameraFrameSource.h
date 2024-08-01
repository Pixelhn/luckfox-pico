#include "FramedSource.hh"
#include "UsageEnvironment.hh"
#include "Groupsock.hh"
#include "GroupsockHelper.hh"

   class CameraFrameSource : public FramedSource {
   public:
      static CameraFrameSource* createNew(UsageEnvironment&);
      CameraFrameSource(UsageEnvironment& env);
      ~CameraFrameSource() = default;
      void onFrame();

   private:
      static void deliverFrameStub(void* clientData) {
         ((CameraFrameSource*)clientData)->deliverFrame();
      };
      void doGetNextFrame() override;
      void deliverFrame();
      void doStopGettingFrames() override;


   private:
      EventTriggerId       fEventTriggerId;
   };

