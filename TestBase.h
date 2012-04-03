#ifndef _TEST_THREAD_H
#define _TEST_THREAD_H

#include <string>
#include <utils/List.h>
#include <utils/threads.h>

#include <surfaceflinger/Surface.h>
#include <surfaceflinger/SurfaceComposerClient.h>
#include <ui/DisplayInfo.h>
#include <ui/GraphicBufferMapper.h>

#include <GLES/gl.h>
#include <GLES/glext.h>

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
        virtual void createSurface();
        bool purgeEglBuffers();
        virtual void chooseEGLConfig(EGLDisplay display, EGLConfig *config);
        virtual EGLContext createEGLContext(EGLDisplay display, EGLConfig config);

        void signalExit();
        bool lockNV12(sp<ANativeWindow> window, ANativeWindowBuffer **b, char **y, char **uv);

        sp<SurfaceSpec> mSpec;

        sp<SurfaceComposerClient> mComposerClient;
        sp<SurfaceControl> mSurfaceControl;
        EGLDisplay mEglDisplay;
        EGLSurface mEglSurface;
        EGLContext mEglContext;
        int mWidth;
        int mHeight;
        int mLastWidth;
        int mLastHeight;


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

        int mLeft;
        int mTop;
        int mLeftStepFactor;
        int mTopStepFactor;
        int mWidthStepFactor;
        int mHeightStepFactor;

        nsecs_t mLastIter;
        Stat mStat;
};

#endif
