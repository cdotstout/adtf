/*
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _TEST_THREAD_H
#define _TEST_THREAD_H

#include <string>
#include <utils/List.h>
#include <utils/threads.h>

#include <ui/DisplayInfo.h>
#include <ui/GraphicBufferMapper.h>

#include <GLES/gl.h>
#include <GLES/glext.h>

#include "LocalTypes.h"
#include "Stat.h"

#define GRALLOC_USAGE       GRALLOC_USAGE_SW_READ_NEVER | \
                            GRALLOC_USAGE_SW_WRITE_OFTEN

using namespace android;

class TestBase : public Thread {
    public:
        TestBase(sp<SurfaceSpec> spec, sp<SurfaceComposerClient> client,
                Mutex &exitLock, Condition &exitCondition);
        virtual ~TestBase();

        sp<SurfaceSpec> getSpec();
        bool done();
        virtual status_t readyToRun();

    protected:
        virtual void updateContent() = 0;
        virtual void createSurface();
        void initEgl();
        bool purgeEglBuffers();
        void freeEgl();
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
        unsigned int mPosCount;
        bool mSteppingPos;
        unsigned int mSizeCount;
        bool mSteppingSize;

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
