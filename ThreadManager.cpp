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

#define LOG_TAG "adtf"

#include "ThreadManager.h"

using namespace android;

ThreadManager::ThreadManager(List<sp<SurfaceSpec> >& specs)
    : Thread(false), mSpecs(specs)
{
}

status_t ThreadManager::readyToRun()
{
    sp<SurfaceComposerClient> composerClient = new SurfaceComposerClient;

    mLock.lock();

    mThreads.clear();
    for (List<sp<SurfaceSpec> >::iterator it = mSpecs.begin(); it != mSpecs.end(); ++it) {
        sp<SurfaceSpec> spec = *it;
        sp<TestBase> thread;
        if (spec->contentType == ContentType::SOLID)
            thread = sp<TestBase>(new SolidThread(spec, composerClient, mLock, mCondition));
        else if (spec->contentType == ContentType::FILE)
            thread = sp<TestBase>(new FileThread(spec, composerClient, mLock, mCondition));
        else if (spec->contentType == ContentType::PLUGIN)
            thread = sp<TestBase>(new PluginThread(spec, composerClient, mLock, mCondition));

        mThreads.push_back(thread);
    }

    mLock.unlock();

    return NO_ERROR;
}

bool ThreadManager::threadLoop()
{
    mLock.lock();

    for (List<sp<TestBase> >::iterator it = mThreads.begin(); it != mThreads.end(); ++it) {
        sp<TestBase> thread = *it;
        thread->run();
    }

    while (mThreads.size() > 0) {
        LOGD("waiting for %i threads", mThreads.size());
        mCondition.wait(mLock);
        List<sp<TestBase> >::iterator it = mThreads.begin();
        while (it != mThreads.end()) {
            sp<TestBase> thread = *it;
            if (thread->done()) {
                mThreads.erase(it);
                mLock.unlock();
                thread->join();
                mLock.lock();
                it = mThreads.begin();
                LOGD("\"%s\" thread exited, keepAlive %d",
                        thread->getSpec()->name.c_str(),
                        thread->getSpec()->renderFlag(RenderFlags::KEEPALIVE));
                if (thread->getSpec()->renderFlag(RenderFlags::KEEPALIVE))
                    mGhosts.push_back(thread);
            } else {
                ++it;
            }
        }
    }

    mLock.unlock();

    mGhosts.clear();

    LOGD("all threads terminated");

    return false;
}
