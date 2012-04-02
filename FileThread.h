#ifndef _FILE_THREAD_H
#define _FILE_THREAD_H

#include <vector>

#include "TestBase.h"

using namespace android;

class FileThread : public TestBase {
    public:
        FileThread(sp<SurfaceSpec> spec, sp<SurfaceComposerClient> client,
                Mutex &exitLock, Condition &exitCondition);
        ~FileThread();
        virtual status_t readyToRun();

    protected:
        virtual void updateContent();
        virtual bool sizeChanged();

    private:
        bool initTexture(void* p);

        int mFd;
        char* mData;
        size_t mLength;
        size_t mFrameSize;
        size_t mFrames;
        size_t mFrameIndex;
        bool mLineByLine;
        int mBpp;
        vector<GLuint> mTIds;
};

#endif
