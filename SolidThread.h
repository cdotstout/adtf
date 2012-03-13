#ifndef _SOLID_THREAD_H
#define _SOLID_THREAD_H

#include <utils/Vector.h>
#include "TestBase.h"

using namespace android;

class SolidThread : public TestBase {
    public:
        SolidThread(sp<SurfaceSpec> spec, sp<SurfaceComposerClient> client,
                Mutex &exitLock, Condition &exitCondition);
        virtual status_t readyToRun();

    protected:
        virtual void updateContent();

    private:
        Vector<unsigned long long> mColors;
        int mBpp;
};

#endif
