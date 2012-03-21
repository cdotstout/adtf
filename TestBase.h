#ifndef _TEST_THREAD_H
#define _TEST_THREAD_H

#include <string>
#include <utils/List.h>
#include <utils/threads.h>

#include <surfaceflinger/Surface.h>
#include <surfaceflinger/SurfaceComposerClient.h>
#include <ui/GraphicBufferMapper.h>

#include "LocalTypes.h"
#include "Stat.h"

#define GRALLOC_USAGE GRALLOC_USAGE_HW_TEXTURE | \
                             GRALLOC_USAGE_HW_RENDER | \
                             GRALLOC_USAGE_SW_READ_RARELY | \
                             GRALLOC_USAGE_SW_WRITE_NEVER

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
        bool lockNV12(sp<ANativeWindow> window, ANativeWindowBuffer **b, char **y, char **uv);

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
