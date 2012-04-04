#define LOG_TAG "adtf"

#include <sstream>
#include <dlfcn.h>

#include "PluginThread.h"

using namespace android;
using namespace std;

PluginThread::PluginThread(sp<SurfaceSpec> spec, sp<SurfaceComposerClient> client,
        Mutex &exitLock, Condition &exitCondition) :
    TestBase(spec, client, exitLock, exitCondition), mHandle(0), mData(0)
{
    // Only GL plugins are supported
    mSpec->renderFlags |= RenderFlags::GL;
    memset(&mFuncs, 0, sizeof(PluginFuncs));
}

PluginThread::~PluginThread()
{
    if (mHandle) {
        if (mFuncs.deinit)
            mFuncs.deinit(mData);
        dlclose(mHandle);
    }
}

status_t PluginThread::readyToRun()
{
    const char *error;
    string lib;
    stringstream ss(stringstream::in | stringstream::out);
    ss.str(mSpec->content);
    ss >> lib;

    LOGD("\"%s\" dlopen '%s'", mSpec->name.c_str(), lib.c_str());

    mHandle = dlopen(lib.c_str(), RTLD_NOW);
    if (!mHandle) {
        LOGE("\"%s\" can't dlopen '%s': %s", mSpec->name.c_str(), lib.c_str(), dlerror());
        signalExit();
        return UNKNOWN_ERROR;
    }

    // Need init and render, the rest are optional
    dlerror();
    mFuncs.init = (int (*)(int, int, void**))dlsym(mHandle, "init");
    if ((error = dlerror()) != NULL || mFuncs.init == NULL)  {
        error = error ? error : "not set: ";
        LOGE("\"%s\" '%s' %sinit", mSpec->name.c_str(), lib.c_str(), error);
        signalExit();
        return UNKNOWN_ERROR;
    }

    dlerror();
    mFuncs.render = (int (*)(void*))dlsym(mHandle, "render");
    if ((error = dlerror()) != NULL || mFuncs.render == NULL)  {
        error = error ? error : "not set: ";
        LOGE("\"%s\" '%s' %srender", mSpec->name.c_str(), lib.c_str(), error);
        signalExit();
        return UNKNOWN_ERROR;
    }

    dlerror();
    mFuncs.deinit = (void (*)(void*))dlsym(mHandle, "deinit");
    if ((error = dlerror()) != NULL || mFuncs.deinit == NULL)  {
        error = error ? error : "not set: ";
        LOGI("\"%s\" '%s' %sdeinit", mSpec->name.c_str(), lib.c_str(), error);
        mFuncs.chooseEGLConfig = NULL;
    }

    dlerror();
    mFuncs.chooseEGLConfig = (int (*)(EGLDisplay, EGLConfig*))dlsym(mHandle, "chooseEGLConfig");
    if ((error = dlerror()) != NULL || mFuncs.chooseEGLConfig == NULL)  {
        error = error ? error : "not set: ";
        LOGI("\"%s\" '%s' %schooseEGLConfig", mSpec->name.c_str(), lib.c_str(), error);
        mFuncs.chooseEGLConfig = NULL;
    }

    dlerror();
    mFuncs.createEGLContext = (EGLContext (*)(EGLDisplay, EGLConfig))dlsym(mHandle, "createEGLContext");
    if ((error = dlerror()) != NULL || mFuncs.createEGLContext == NULL)  {
        error = error ? error : "not set: ";
        LOGI("\"%s\" '%s' %screateEGLContext", mSpec->name.c_str(), lib.c_str(), error);
        mFuncs.createEGLContext = NULL;
    }

    dlerror();
    mFuncs.sizeChanged = (int (*)(void*, int, int))dlsym(mHandle, "sizeChanged");
    if ((error = dlerror()) != NULL || mFuncs.sizeChanged == NULL)  {
        error = error ? error : "not set: ";
        LOGI("\"%s\" '%s' %ssizeChanged", mSpec->name.c_str(), lib.c_str(), error);
        mFuncs.sizeChanged = NULL;
    }

    LOGD("\"%s\" plugin functions loaded", mSpec->name.c_str());

    createSurface();
    if (mSurfaceControl == 0 || done()) {
        LOGE("\"%s\" failed to create surface", mSpec->name.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    LOGD("\"%s\" calling plugin init", mSpec->name.c_str());
    int ret;
    if ((ret = mFuncs.init(mWidth, mHeight, &mData)) != 0) {
        LOGE("\"%s\" plugin init failed, %d", mSpec->name.c_str(), ret);
        signalExit();
        return UNKNOWN_ERROR;
    }

    return TestBase::readyToRun();
}

void PluginThread::chooseEGLConfig(EGLDisplay display, EGLConfig *config)
{
    if (mFuncs.chooseEGLConfig) {
        mFuncs.chooseEGLConfig(display, config);
        if (*config == NULL) {
            LOGE("\"%s\" plugin chooseEGLConfig malfunction, using default", mSpec->name.c_str());
            TestBase::chooseEGLConfig(display, config);
        }
    } else
        TestBase::chooseEGLConfig(display, config);
}

EGLContext PluginThread::createEGLContext(EGLDisplay display, EGLConfig config)
{
    if (mFuncs.createEGLContext)
        return mFuncs.createEGLContext(display, config);

    return TestBase::createEGLContext(display, config);
}

void PluginThread::updateContent()
{
    int ret;

    if (mWidth != mLastWidth || mHeight != mLastHeight) {
        // Render once with the old dimensions
        if ((ret = mFuncs.render(mData)) != 0) {
            LOGE("\"%s\" plugin render failed, %d", mSpec->name.c_str(), ret);
            requestExit();
            return;
        }
        eglSwapBuffers(mEglDisplay, mEglSurface);

        // Purge buffers
        if (!TestBase::purgeEglBuffers()) {
            requestExit();
            return;
        }

        // Tell plugin that size changed
        if (mFuncs.sizeChanged) {
            if ((ret = mFuncs.sizeChanged(mData, mWidth, mHeight) != 0)) {
                LOGE("\"%s\" plugin render sizeChanged failed, %d", mSpec->name.c_str(), ret);
                requestExit();
                return;
            }
        }
    }

    if ((ret = mFuncs.render(mData)) != 0) {
        LOGE("\"%s\" plugin render failed, %d", mSpec->name.c_str(), ret);
        requestExit();
        return;
    }
    eglSwapBuffers(mEglDisplay, mEglSurface);
}
