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

#include <sstream>
#include <vector>
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
        if (mFuncs.destroy) {
            LOGD("\"%s\" calling plugin destroy", mSpec->name.c_str());
            mFuncs.destroy(mData);
        }
        dlclose(mHandle);
    }
}

status_t PluginThread::readyToRun()
{
    const char *error;
    string lib;
    string param;
    vector<string> params;

    stringstream ss(stringstream::in | stringstream::out);
    ss.str(mSpec->content);
    ss >> lib;

    // This parameter extraction does NOT support quotations or other fancy stuff
    params.push_back(lib); // argv[0]
    ss >> param;
    while (!ss.fail()) {
        params.push_back(param); // argv[i]
        ss >> param;
    }
    int argc = params.size();
    char *argv[argc + 1];
    for (int i = 0; i < argc; i++)
        argv[i] = (char*)params[i].c_str();
    argv[argc] = 0;

    LOGD("\"%s\" dlopen '%s'", mSpec->name.c_str(), lib.c_str());

    mHandle = dlopen(lib.c_str(), RTLD_NOW);
    if (!mHandle) {
        LOGE("\"%s\" can't dlopen '%s': %s", mSpec->name.c_str(), lib.c_str(), dlerror());
        signalExit();
        return UNKNOWN_ERROR;
    }

    // Need create and render, the rest are optional
    dlerror();
    mFuncs.create = (int (*)(int, int, int, char**, void**))dlsym(mHandle, "create");
    if ((error = dlerror()) != NULL || mFuncs.create == NULL)  {
        error = error ? error : "not set: ";
        LOGE("\"%s\" '%s' %screate", mSpec->name.c_str(), lib.c_str(), error);
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
    mFuncs.init = (int (*)(void*))dlsym(mHandle, "init");
    if ((error = dlerror()) != NULL || mFuncs.init == NULL)  {
        error = error ? error : "not set: ";
        LOGI("\"%s\" '%s' %sinit", mSpec->name.c_str(), lib.c_str(), error);
        mFuncs.init = NULL;
    }

    dlerror();
    mFuncs.destroy = (void (*)(void*))dlsym(mHandle, "destroy");
    if ((error = dlerror()) != NULL || mFuncs.destroy == NULL)  {
        error = error ? error : "not set: ";
        LOGI("\"%s\" '%s' %sdestroy", mSpec->name.c_str(), lib.c_str(), error);
        mFuncs.chooseEGLConfig = NULL;
    }

    dlerror();
    mFuncs.chooseEGLConfig = (int (*)(void*, EGLDisplay, EGLConfig*))dlsym(mHandle, "chooseEGLConfig");
    if ((error = dlerror()) != NULL || mFuncs.chooseEGLConfig == NULL)  {
        error = error ? error : "not set: ";
        LOGI("\"%s\" '%s' %schooseEGLConfig", mSpec->name.c_str(), lib.c_str(), error);
        mFuncs.chooseEGLConfig = NULL;
    }

    dlerror();
    mFuncs.createEGLContext = (EGLContext (*)(void*, EGLDisplay, EGLConfig))dlsym(mHandle, "createEGLContext");
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

    createSurface();
    if (mSurfaceControl == 0 || done()) {
        LOGE("\"%s\" failed to create surface", mSpec->name.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    LOGD("\"%s\" plugin functions loaded, calling create", mSpec->name.c_str());

    int ret;
    if ((ret = mFuncs.create(mWidth, mHeight, argc, argv, &mData)) != 0) {
        LOGE("\"%s\" plugin create failed, %d", mSpec->name.c_str(), ret);
        signalExit();
        return UNKNOWN_ERROR;
    }

    TestBase::initEgl();
    if (done()) {
        LOGE("\"%s\" initEgl failed", mSpec->name.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    if (mFuncs.init) {
        LOGD("\"%s\" calling plugin init", mSpec->name.c_str());
        if ((ret = mFuncs.init(mData)) != 0) {
            LOGE("\"%s\" plugin init failed, %d", mSpec->name.c_str(), ret);
            signalExit();
            return UNKNOWN_ERROR;
        }
    }

    return TestBase::readyToRun();
}

void PluginThread::chooseEGLConfig(EGLDisplay display, EGLConfig *config)
{
    if (mFuncs.chooseEGLConfig) {
        mFuncs.chooseEGLConfig(mData, display, config);
        if (*config == NULL) {
            LOGI("\"%s\" plugin chooseEGLConfig returned NULL, using default", mSpec->name.c_str());
            TestBase::chooseEGLConfig(display, config);
        }
    } else
        TestBase::chooseEGLConfig(display, config);
}

EGLContext PluginThread::createEGLContext(EGLDisplay display, EGLConfig config)
{
    EGLContext res = NULL;
    if (mFuncs.createEGLContext) {
        res = mFuncs.createEGLContext(mData, display, config);
        if (res == NULL)
            LOGI("\"%s\" plugin createEGLContext returned NULL, using default", mSpec->name.c_str());
    }

    if (res == NULL)
        res = TestBase::createEGLContext(display, config);

    return res;
}

// If this function returns false caller must abort
bool PluginThread::pluginRender(bool& swap)
{
    swap = true;
    int ret = mFuncs.render(mData);

    // 0 : All is well, carry on
    if (ret == 0)
        return true;

    // 1: Request for reinit
    if (ret == 1) {
        LOGD("\"%s\" plugin render requested reinit %d", mSpec->name.c_str(), ret);

        // Do reinit
        TestBase::freeEgl();
        TestBase::initEgl();
        // Call render again with the new config
        ret = mFuncs.render(mData);

        // Fall through. Another reinit request is intentionally not handled.
    }

    // 2: Abort, nothing more to do (not an error)
    if (ret == 2) {
        LOGD("\"%s\" plugin render says it's done", mSpec->name.c_str());
        return false;
    }

    // 3: OK, but skip swap
    if (ret == 3) {
        swap = false;
        return true;
    }

    // Generic cases. Positive == warning, negative == error
    if (ret == 0)
        return true;
    else if (ret > 0) {
        LOGW("\"%s\" plugin render unknown result %d", mSpec->name.c_str(), ret);
        return true;
    } else {
        LOGE("\"%s\" plugin render returned %d", mSpec->name.c_str(), ret);
        requestExit();
        return false;
    }
}

void PluginThread::updateContent()
{
    bool swap = true;

    if (mWidth != mLastWidth || mHeight != mLastHeight) {
        // Render once with the old dimensions
        if (!pluginRender(swap)) {
            requestExit();
            return;
        }
        if (swap)
            eglSwapBuffers(mEglDisplay, mEglSurface);

        // Purge buffers
        if (!TestBase::purgeEglBuffers()) {
            requestExit();
            return;
        }

        // Tell plugin that size changed
        if (mFuncs.sizeChanged) {
            LOGD("\"%s\" %dx%d --> %dx%d, telling plugin", mSpec->name.c_str(),
                    mLastWidth, mLastHeight, mWidth, mHeight);
            int ret;
            if ((ret = mFuncs.sizeChanged(mData, mWidth, mHeight) != 0)) {
                LOGE("\"%s\" plugin render sizeChanged failed, %d", mSpec->name.c_str(), ret);
                requestExit();
                return;
            }
        }
    }

    if (!pluginRender(swap)) {
        requestExit();
        return;
    }
    if (swap)
       eglSwapBuffers(mEglDisplay, mEglSurface);
}
