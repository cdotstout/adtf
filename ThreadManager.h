#include "FileThread.h"
#include "SolidThread.h"
#include "PluginThread.h"

using namespace android;

class ThreadManager : public Thread {
    public:
        ThreadManager(List<sp<SurfaceSpec> >& specs);

        status_t readyToRun();

    private:
        bool threadLoop();

        Mutex mLock;
        Condition mCondition;

        List<sp<SurfaceSpec> > mSpecs;
        List<sp<TestBase> > mThreads;
        List<sp<TestBase> > mGhosts;
};
