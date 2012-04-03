#ifndef _PLUGIN_THREAD_H
#define _PLUGIN_THREAD_H

#include <utils/Vector.h>
#include "TestBase.h"

using namespace android;

class PluginThread : public TestBase {

    struct PluginFuncs {    
        int (*init)(int, int, void**);
        void (*deinit)(void*);
        int (*chooseEGLConfig)(EGLDisplay, EGLConfig*);
        EGLContext (*createEGLContext)(EGLDisplay, EGLConfig);
        int (*render)(void*);
        int (*sizeChanged)(void*, int, int);
    };

    public:
        PluginThread(sp<SurfaceSpec> spec, sp<SurfaceComposerClient> client,
                Mutex &exitLock, Condition &exitCondition);
        ~PluginThread();
        virtual status_t readyToRun();

    protected:
        virtual void updateContent();
        virtual void chooseEGLConfig(EGLDisplay display, EGLConfig *config);
        virtual EGLContext createEGLContext(EGLDisplay display, EGLConfig config);

    private:
        void *mHandle;
        void *mData;
        PluginFuncs mFuncs;
};

#endif
