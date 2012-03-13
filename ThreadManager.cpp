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
        if (spec->contentType == SOLID_FILL)
            thread = sp<TestBase>(new SolidThread(spec, composerClient, mLock, mCondition));
        else if (spec->contentType == LOCAL_FILE)
            thread = sp<TestBase>(new FileThread(spec, composerClient, mLock, mCondition));

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
        status_t status = thread->run();
    }

    while (mThreads.size() > 0) {
        LOGD("waiting for %i threads", mThreads.size());
        mCondition.wait(mLock);
        for (List<sp<TestBase> >::iterator it = mThreads.begin(); it != mThreads.end(); ++it) {
            sp<TestBase> thread = *it;
            if (thread->done()) {
                it = mThreads.erase(it); // TODO: Put thread in separate list to keep instance alive (final surface showing until all is done)
                thread->join();
                LOGD("\"%s\" thread exited, keepAlive %s",
                        thread->getSpec()->name.c_str(),
                        (thread->getSpec()->keepAlive ? "true": "false"));
                if (thread->getSpec()->keepAlive)
                    mGhosts.push_back(thread);
            }
        }
    }

    mLock.unlock();

    mGhosts.clear();

    LOGD("all threads terminated");

    return false;
}
