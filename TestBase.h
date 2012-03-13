#ifndef _TEST_THREAD_H
#define _TEST_THREAD_H

#include <string>
#include <utils/List.h>
#include <utils/threads.h>

#include <surfaceflinger/Surface.h>
#include <surfaceflinger/SurfaceComposerClient.h>

#include "LocalTypes.h"
#include "Stat.h"

using namespace android;

class TestBase : public Thread {
    public:
        TestBase(sp<SurfaceSpec> spec, sp<SurfaceComposerClient> client,
                Mutex &exitLock, Condition &exitCondition);
         ~TestBase();

        sp<SurfaceSpec> getSpec();
        bool done();
        virtual status_t readyToRun();

    protected:
        virtual void updateContent() = 0;
        void signalExit();
        void createSurface();

        sp<SurfaceSpec> mSpec;

        sp<SurfaceComposerClient> mComposerClient;
        sp<SurfaceControl> mSurfaceControl;

    private:
        int getVisibility();
        bool updatePosition();
        bool updateSize();
        bool updateContent(bool force);
        bool threadLoop();

        Mutex &mExitLock;
        Condition &mExitCondition;

        unsigned int mUpdateCount;
        bool mUpdating;
        unsigned int mVisibleCount;
        bool mVisible;

        int mWidth;
        int mHeight;
        int mLeft;
        int mTop;
        nsecs_t mLastIter;
        Stat mStat;
};

#endif
