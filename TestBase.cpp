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

#include <unistd.h>
#include <vector>

#include "TestBase.h"

using namespace android;

TestBase::TestBase(sp<SurfaceSpec> spec, sp<SurfaceComposerClient> client,
        Mutex &exitLock, Condition &exitCondition) :
    Thread(false), mSpec(spec), mComposerClient(client), mSurfaceControl(0),
    mEglDisplay(EGL_NO_DISPLAY), mEglSurface(0), mEglContext(0),
    mExitLock(exitLock), mExitCondition(exitCondition), mUpdateCount(0),
    mUpdating(true), mVisibleCount(0), mVisible(false), mPosCount(0),
    mSteppingPos(true), mSizeCount(0), mSteppingSize(true), mLeftStepFactor(1),
    mTopStepFactor(1), mWidthStepFactor(1), mHeightStepFactor(1)
{
#ifndef ADTF_ICS_AND_EARLIER
    mEventReceiver = NULL;
#endif
    LOGD("\"%s\" thread created", mSpec->name.c_str());
}

TestBase::~TestBase()
{
    freeEgl();

#ifndef ADTF_ICS_AND_EARLIER
    if (mEventReceiver)
        delete mEventReceiver;
#endif

    LOGD("\"%s\" thread going away", mSpec->name.c_str());
}

sp<SurfaceSpec> TestBase::getSpec()
{
    return mSpec;
}

bool TestBase::done()
{
    return exitPending();
}

status_t TestBase::readyToRun()
{
    status_t status = NO_ERROR;

    status = mComposerClient->initCheck();
    if (status != NO_ERROR) {
        LOGE("\"%s\" failed composer init check", mSpec->name.c_str());
        signalExit();
        return status;
    }

    if (mSurfaceControl == 0) {
        status = UNKNOWN_ERROR;
        LOGE("\"%s\" subclass failed to create surface", mSpec->name.c_str());
        signalExit();
        return status;
    }

#ifndef ADTF_ICS_AND_EARLIER
    if (mSpec->renderFlag(RenderFlags::VSYNC)) {
        if (!mEventReceiver)
            mEventReceiver = new DisplayEventReceiver();
        status = mEventReceiver->initCheck();
        if (status != NO_ERROR) {
            LOGE("\"%s\" failed DisplayEventReceiver init check", mSpec->name.c_str());
            signalExit();
            return status;
        }
        mEventLooper = new Looper(true);
        mEventLooper->addFd(mEventReceiver->getFd(), 0, ALOOPER_EVENT_INPUT, 0, 0);
        status = mEventReceiver->requestNextVsync();
        if (status != NO_ERROR) {
            LOGE("\"%s\" failed to request vsync", mSpec->name.c_str());
            signalExit();
            return status;
        }
    }
#endif

    if (done()) {
        status = UNKNOWN_ERROR;
        signalExit();
        return status;
    }

    mStat.clear();

    sp<Surface> surface = mSurfaceControl->getSurface();
    sp<ANativeWindow> window(surface);
    ANativeWindow *w = window.get();

    if (!mSpec->renderFlag(RenderFlags::GL)) {
        if (mSpec->renderFlag(RenderFlags::ASYNC)) {
            status |= native_window_api_connect(w, NATIVE_WINDOW_API_MEDIA);
            if (status != 0) {
                LOGE("\"%s\" failed to set async mode", mSpec->name.c_str());
                signalExit();
                return status;
            }
        }

        if (mSpec->bufferFormat != mSpec->format) {
            int min = 0;
            status |= w->query(w, NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &min);
            status |= native_window_set_usage(w, GRALLOC_USAGE);
            status |= native_window_set_buffer_count(w, min + 1);
            status |= native_window_set_buffers_format(w, mSpec->bufferFormat);
            if (status != 0) {
                LOGE("\"%s\" failed to configure buffer usage/count/format", mSpec->name.c_str());
                signalExit();
                return status;
            }
        }
    }

    // Default is to dequeue buffers with same dimensions as the surface.
    // Override this, but it will silently be reverted by the GL driver if used
    status |= native_window_set_buffers_dimensions(w, mSpec->srcGeometry.width, mSpec->srcGeometry.height);
    status |= native_window_set_scaling_mode(w, NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);
    if (status != 0) {
        LOGE("\"%s\" failed to configure buffer dimension/scaling", mSpec->name.c_str());
        signalExit();
        return status;
    }

    if (mSpec->srcGeometry.crop.isValid()) {
        android_native_rect_t c;
        c.left = mSpec->srcGeometry.crop.left;
        c.top = mSpec->srcGeometry.crop.top;
        c.right = mSpec->srcGeometry.crop.right;
        c.bottom = mSpec->srcGeometry.crop.bottom;
        status |= native_window_set_crop(w, &c);
        if (status != 0) {
            LOGE("\"%s\" failed to set crop", mSpec->name.c_str());
            signalExit();
            return status;
        }
    }

    if (mSpec->transform != 0) {
        LOGE("\"%s\" setting transform %d", mSpec->name.c_str(), mSpec->transform);
        status |= native_window_set_buffers_transform(w, mSpec->transform);
        if (status != 0) {
            LOGE("\"%s\" failed to set transform", mSpec->name.c_str());
            signalExit();
            return status;
        }
    }

    SurfaceComposerClient::openGlobalTransaction();
    mStat.openTransaction();

    if (mSpec->transparentRegionHint.size() > 0) {
        Region r;
        String8 str;

        r.set(mWidth, mHeight);
        for (List<Rect>::iterator it = mSpec->transparentRegionHint.begin();
                it != mSpec->transparentRegionHint.end(); ++it)
            r.addRectUnchecked((*it).left, (*it).top, (*it).right, (*it).bottom);

        r.dump(str, "transparentRegion", 0);
        LOGD("\"%s\" %s", mSpec->name.c_str(), str.string());
        status |= mSurfaceControl->setTransparentRegionHint(r);
        if (status != 0) {
            LOGE("\"%s\" setTransparentRegionHint failed", mSpec->name.c_str());
            signalExit();
            return status;
        }
    }

    mSurfaceControl->setLayer(mSpec->zOrder);
    mLeft = mSpec->outRect.left;
    mTop = mSpec->outRect.top;
    mSurfaceControl->setPosition(mLeft, mTop);
    mStat.setPosition();

    SurfaceComposerClient::closeGlobalTransaction();
    mStat.closeTransaction();

    mLastWidth = mWidth;
    mLastHeight = mHeight;

    mStat.startUpdate();
    updateContent(true); // Resets update duty cycle
    updateContent(); // Always perform at least one update
    if (done()) {
        status = UNKNOWN_ERROR;
        LOGE("\"%s\" failed to update content", mSpec->name.c_str());
        signalExit();
        return status;
    }
    mStat.doneUpdate();

    mVisible = (mSpec->flags & ISurfaceComposer::eHidden) == 0;
    mVisibleCount = 0;
    mLastIter = systemTime();

    return NO_ERROR;
}

void TestBase::createSurface()
{
    if (mSurfaceControl != 0)
        return;

    int w = mSpec->outRect.width();
    int h = mSpec->outRect.height();

    if (w <= 0)
        w = mSpec->srcGeometry.width;
    if (h <= 0)
        h = mSpec->srcGeometry.height;

    if (w <= 0 || h <= 0) {
        LOGE("\"%s\" invalid source geometry %dx%d", mSpec->name.c_str(), w, h);
        signalExit();
        return;
    }

    mSurfaceControl = mComposerClient->createSurface(
            String8(mSpec->name.c_str()),
            0, // TODO: DisplayID
            w,
            h,
            mSpec->format,
            mSpec->flags
    );
    mWidth = w;
    mHeight = h;
}

void TestBase::initEgl()
{
    if (!mSpec->renderFlag(RenderFlags::GL))
        return;

    EGLConfig config = NULL;
    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(mEglDisplay, 0, 0);
    chooseEGLConfig(mEglDisplay, &config);
    if (config == NULL) {
        LOGE("\"%s\" chooseEGLConfig failed", mSpec->name.c_str());
        signalExit();
        return;
    }

    // 33 attributes used for debugging
    int attr[] = {
        EGL_BUFFER_SIZE, EGL_ALPHA_SIZE, EGL_BLUE_SIZE, EGL_GREEN_SIZE,
        EGL_RED_SIZE, EGL_DEPTH_SIZE, EGL_STENCIL_SIZE, EGL_CONFIG_CAVEAT,
        EGL_CONFIG_ID, EGL_LEVEL, EGL_MAX_PBUFFER_HEIGHT,
        EGL_MAX_PBUFFER_PIXELS, EGL_MAX_PBUFFER_WIDTH, EGL_NATIVE_RENDERABLE,
        EGL_NATIVE_VISUAL_ID, EGL_NATIVE_VISUAL_TYPE,
        0x3030, // EGL_PRESERVED_RESOURCES
        EGL_SAMPLES, EGL_SAMPLE_BUFFERS, EGL_SURFACE_TYPE,
        EGL_TRANSPARENT_TYPE, EGL_TRANSPARENT_RED_VALUE,
        EGL_TRANSPARENT_GREEN_VALUE, EGL_TRANSPARENT_BLUE_VALUE,
        0x3039, // EGL_BIND_TO_TEXTURE_RGB
        0x303A, // EGL_BIND_TO_TEXTURE_RGBA,
        0x303B, // EGL_MIN_SWAP_INTERVAL,
        0x303C, // EGL_MAX_SWAP_INTERVAL,
        EGL_LUMINANCE_SIZE, EGL_ALPHA_MASK_SIZE, EGL_COLOR_BUFFER_TYPE,
        EGL_RENDERABLE_TYPE,
        0x3042 // EGL_CONFORMANT
    };

    const char* attr_names[] = {
        "EGL_BUFFER_SIZE", "EGL_ALPHA_SIZE", "EGL_BLUE_SIZE", "EGL_GREEN_SIZE",
        "EGL_RED_SIZE", "EGL_DEPTH_SIZE", "EGL_STENCIL_SIZE", "EGL_CONFIG_CAVEAT",
        "EGL_CONFIG_ID", "EGL_LEVEL", "EGL_MAX_PBUFFER_HEIGHT",
        "EGL_MAX_PBUFFER_PIXELS", "EGL_MAX_PBUFFER_WIDTH", "EGL_NATIVE_RENDERABLE",
        "EGL_NATIVE_VISUAL_ID", "EGL_NATIVE_VISUAL_TYPE", "EGL_PRESERVED_RESOURCES",
        "EGL_SAMPLES", "EGL_SAMPLE_BUFFERS", "EGL_SURFACE_TYPE", "EGL_TRANSPARENT_TYPE",
        "EGL_TRANSPARENT_RED_VALUE", "EGL_TRANSPARENT_GREEN_VALUE",
        "EGL_TRANSPARENT_BLUE_VALUE", "EGL_BIND_TO_TEXTURE_RGB",
        "EGL_BIND_TO_TEXTURE_RGBA", "EGL_MIN_SWAP_INTERVAL", "EGL_MAX_SWAP_INTERVAL",
        "EGL_LUMINANCE_SIZE", "EGL_ALPHA_MASK_SIZE", "EGL_COLOR_BUFFER_TYPE",
        "EGL_RENDERABLE_TYPE", "EGL_CONFORMANT"
    };

    EGLint value;
    for (int j = 0; j < 33; j++) {
        if (eglGetConfigAttrib(mEglDisplay, config, attr[j], &value) == EGL_TRUE)
            LOGD("\"%s\" EGLConfig %s : %d", mSpec->name.c_str(), attr_names[j], value);
        else
            while (eglGetError() != EGL_SUCCESS);
    }

    mEglSurface = eglCreateWindowSurface(mEglDisplay, config,
            mSurfaceControl->getSurface().get(), NULL);
    mEglContext = createEGLContext(mEglDisplay, config);
    if (mEglContext == 0) {
        LOGE("\"%s\" createEGLContext failed", mSpec->name.c_str());
        signalExit();
        return;
    }

    if (eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext) == EGL_FALSE) {
        LOGE("\"%s\" eglMakeCurrent failed", mSpec->name.c_str());
        signalExit();
    }
}

bool TestBase::purgeEglBuffers()
{
    if (mSpec->renderFlag(RenderFlags::GL)) {

        if (eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE) {
            LOGE("\"%s\" eglMakeCurrent failed", mSpec->name.c_str());
            requestExit();
            return false;
        }

        if (eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext) == EGL_FALSE) {
            LOGE("\"%s\" eglMakeCurrent failed", mSpec->name.c_str());
            requestExit();
            return false;
        }
    }

    return true;
}

void TestBase::freeEgl()
{
    if (mEglDisplay == EGL_NO_DISPLAY)
        return;

    if (mEglSurface != 0) {
        eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(mEglDisplay, mEglSurface);
        mEglSurface = 0;
    }
    if (mEglContext != 0) {
        eglDestroyContext(mEglDisplay, mEglContext);
        mEglContext = 0;
    }
    eglTerminate(mEglDisplay);
    mEglDisplay = EGL_NO_DISPLAY;
}

// Unless overridden, choose config matching surface format, GLES 1
void TestBase::chooseEGLConfig(EGLDisplay display, EGLConfig *config)
{
    int a, r, g, b;
    a = r = g = b = 0;
    *config = NULL;

    // Note: Non explicit formats are translated in SurfaceFlinger::createNormalSurface
    // If translation changes there, this must be updated. Use explicit formats to avoid problems
    switch (mSpec->format) {
        // Translated to PIXEL_FORMAT_RGBA_8888
        case PIXEL_FORMAT_TRANSLUCENT:
        case PIXEL_FORMAT_TRANSPARENT:
        case PIXEL_FORMAT_RGBA_8888:
        case PIXEL_FORMAT_BGRA_8888:
            a = r = g = b = 8;
            break;

        case PIXEL_FORMAT_OPAQUE:
            // Warning: This one is board platform specific and should not be used!
            // Translated to PIXEL_FORMAT_RGBX_8888 for most
            LOGW("\"%s\" PIXEL_FORMAT_OPAQUE used for EGL, treating as PIXEL_FORMAT_RGBX_8888", mSpec->name.c_str());
        case PIXEL_FORMAT_RGBX_8888:
        case PIXEL_FORMAT_RGB_888:
            r = g = b = 8;
            a = 0;
        break;

        case PIXEL_FORMAT_RGB_565:
            r = b = 5;
            g = 6;
            a = 0;
            break;
        case PIXEL_FORMAT_RGBA_5551:
            r = g = b = 5;
            a = 1;
            break;
        case PIXEL_FORMAT_RGBA_4444:
            a = r = g = b = 4;
            break;

        case PIXEL_FORMAT_NONE:
        case PIXEL_FORMAT_CUSTOM:
        default:
            LOGE("\"%s\" invalid format", mSpec->name.c_str());
            return;
    }

    const EGLint attribs[] = {
        EGL_RED_SIZE,   r,
        EGL_GREEN_SIZE, g,
        EGL_BLUE_SIZE,  b,
        EGL_ALPHA_SIZE, a,
        EGL_NONE
    };

    LOGD("\"%s\" calling eglChooseConfig with sizes r=%d g=%d b=%d a=%d",
            mSpec->name.c_str(), r, g, b, a);

    EGLint num;
    vector<EGLConfig> v;

    eglChooseConfig(display, attribs, NULL, 0, &num);
    v.reserve(num);
    eglChooseConfig(display, attribs, &v[0], num, &num);

    LOGD("\"%s\" eglChooseConfig returned %d configs", mSpec->name.c_str(), num);

    // Take the first found config that matches exactly
    for (int i = 0; i < num; i++) {
        EGLint rv = -1, gv = -1, bv = -1, av = -1;
        eglGetConfigAttrib(mEglDisplay, v[i], EGL_RED_SIZE, &rv);
        eglGetConfigAttrib(mEglDisplay, v[i], EGL_GREEN_SIZE, &gv);
        eglGetConfigAttrib(mEglDisplay, v[i], EGL_BLUE_SIZE, &bv);
        eglGetConfigAttrib(mEglDisplay, v[i], EGL_ALPHA_SIZE, &av);
        if (rv == r && gv == g && bv == b && av == a) {
            LOGD("\"%s\" using config %d with sizes r=%d g=%d b=%d a=%d",
                mSpec->name.c_str(), i, rv, gv, bv, av);
            memcpy(config, &v[i], sizeof(EGLConfig));
            return;
        }
    }

    // No exact match, warn and return first one
    LOGW("\"%s\" no matching EGLConfig, using default", mSpec->name.c_str());
    eglChooseConfig(display, attribs, config, 1, &num);
}

// Must be overridden for GLES 2
EGLContext TestBase::createEGLContext(EGLDisplay display, EGLConfig config)
{
    return eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
}

bool TestBase::lockNV12(sp<ANativeWindow> window, ANativeWindowBuffer **b, char **y, char **uv)
{
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    ANativeWindow *w = window.get();
    Rect bounds(0, 0, mSpec->srcGeometry.width, mSpec->srcGeometry.height);
    void *d[2];
    d[0] = d[1] = 0;

    int res = 0;
    res = w->dequeueBuffer(w, b);
    if (res != 0) {
        LOGE("\"%s\" dequeueBuffer failed", mSpec->name.c_str());
        return false;
    }

    res = w->lockBuffer(w, *b);
    if (res != 0) {
        LOGE("\"%s\" lockBuffer failed", mSpec->name.c_str());
        w->cancelBuffer(w, *b);
        return false;
    }

    res = mapper.lock((*b)->handle, GRALLOC_USAGE, bounds, d);
    if (res != 0) {
        LOGE("\"%s\" mapper.lock failed", mSpec->name.c_str());
        w->cancelBuffer(w, *b);
        return false;
    }

    *y = (char*)d[0];
    *uv = (char*)d[1];

    if (*y == 0) {
        LOGE("\"%s\" y pointer invalid", mSpec->name.c_str());
        mapper.unlock((*b)->handle);
        w->cancelBuffer(w, *b);
        return false;
    }

    if (*uv == 0)
        *uv = *y + (*b)->height * (*b)->stride;

    return true;
}

bool TestBase::updateContent(bool force)
{
    UpdateParams p = mSpec->updateParams;

    // Never update unless forced
    if (p.contentUpdateCycle.onCount == 0)
        return force;

    // Always update
    if (p.contentUpdateCycle.offCount == 0)
        return true;

    if (force && !mUpdating) {
        // Start over
        mUpdating = true;
        mUpdateCount = 0;
    }

    mUpdateCount++;

    if (mUpdating || force) {
        mUpdating = mUpdateCount < p.contentUpdateCycle.onCount;
        if (!mUpdating)
            mUpdateCount = 0; // Next iteration is first in off cycle

        return true;
    }

    if (mUpdateCount < p.contentUpdateCycle.offCount)
        return false;

    // Last iteration in off cycle, next is first in on cycle
    mUpdating = true;
    mUpdateCount = 0;

    return false;
}

int TestBase::getVisibility()
{
    // Negative=hide, 0=no change, positive=show

    UpdateParams p = mSpec->updateParams;

    // Check if always visible
    if (p.showCycle.onCount == 0 ||
            p.showCycle.offCount == 0) {
        if (mVisible)
            return 0;
        return 1;
    }

    mVisibleCount++;

    if (mVisible) {
        mVisible = mVisibleCount < p.showCycle.onCount;
        if (!mVisible) {
            mVisibleCount = 0;
            return -1;
        }
        return 0;
    }

    mVisible = mVisibleCount >= p.showCycle.offCount;
    if (mVisible) {
        mVisibleCount = 0;
        return 1;
    }
    return 0;
}

bool TestBase::updatePosition()
{
    bool changed = false;
    bool doStep = false;
    UpdateParams p = mSpec->updateParams;

    // Never update
    if (p.positionCycle.onCount == 0)
        return false;

    // Always update
    if (p.positionCycle.offCount == 0)
        mSteppingPos = true;

    mPosCount++;

    if (mSteppingPos) {
        doStep = true;
        mSteppingPos = mPosCount < p.positionCycle.onCount;
        mPosCount = mSteppingPos ? mPosCount : 0;
    } else if (mPosCount >= p.positionCycle.offCount) {
        // Last iteration in off cycle, next is first in on cycle
        mSteppingPos = true;
        mPosCount = 0;
    }

    if (!doStep)
        return false;

    if (p.outRectStep.left != 0) {
        int minVal = min(mSpec->outRect.left, p.outRectLimit.left);
        int maxVal = max(mSpec->outRect.left, p.outRectLimit.left);

        mLeft += mLeftStepFactor * p.outRectStep.left;
        if (mLeft < minVal || mLeft > maxVal) {
            mLeftStepFactor *= -1;
            mLeft += 2 * mLeftStepFactor * p.outRectStep.left;
        }
        mLeft = min(mLeft, maxVal);
        mLeft = max(mLeft, minVal);
        changed = true;
    }

    if (p.outRectStep.top != 0) {
        int minVal = min(mSpec->outRect.top, p.outRectLimit.top);
        int maxVal = max(mSpec->outRect.top, p.outRectLimit.top);

        mTop += mTopStepFactor * p.outRectStep.top;
        if (mTop < minVal || mTop > maxVal) {
            mTopStepFactor *= -1;
            mTop += 2 * mTopStepFactor * p.outRectStep.top;
        }
        mTop = min(mTop, maxVal);
        mTop = max(mTop, minVal);
        changed = true;
    }

    return changed;
}

bool TestBase::updateSize()
{
    bool changed = false;
    bool doStep = false;
    UpdateParams p = mSpec->updateParams;
    Rect lim = p.outRectLimit;
    int ow = mSpec->outRect.width();
    int oh = mSpec->outRect.height();
    if (ow <= 0)
        ow = mSpec->srcGeometry.width;
    if (oh <= 0)
        oh = mSpec->srcGeometry.height;

    int dw = p.outRectStep.width();
    int dh = p.outRectStep.height();

    // Never update
    if (p.sizeCycle.onCount == 0)
        return false;

    // Always update
    if (p.sizeCycle.offCount == 0)
        mSteppingSize = true;

    mSizeCount++;

    if (mSteppingSize) {
        doStep = true;
        mSteppingSize = mSizeCount < p.sizeCycle.onCount;
        mSizeCount = mSteppingSize ? mSizeCount : 0;
    } else if (mSizeCount >= p.sizeCycle.offCount) {
        // Last iteration in off cycle, next is first in on cycle
        mSteppingSize = true;
        mSizeCount = 0;
    }

    if (!doStep)
        return false;

    if (dw != 0 && lim.width() > 0) {
        int minVal = min(ow, lim.width());
        int maxVal = max(ow, lim.width());

        mWidth += mWidthStepFactor * dw;
        if (mWidth < minVal || mWidth > maxVal) {
            mWidthStepFactor *= -1;
            mWidth += 2 * mWidthStepFactor * dw;
        }
        mWidth = min(mWidth, maxVal);
        mWidth = max(mWidth, minVal);
        changed = true;
    }

    if (dh != 0 && lim.height() > 0) {
        int minVal = min(oh, lim.height());
        int maxVal = max(oh, lim.height());

        mHeight += mHeightStepFactor * dw;
        if (mHeight < minVal || mHeight > maxVal) {
            mHeightStepFactor *= -1;
            mHeight += 2 * mHeightStepFactor * dw;
        }
        mHeight = min(mHeight, maxVal);
        mHeight = max(mHeight, minVal);
        changed = true;
    }

    return changed;
}

void TestBase::signalExit()
{
    requestExit();
    mExitLock.lock();
    mExitCondition.broadcast();
    mExitLock.unlock();
}

bool TestBase::threadLoop()
{
    LOGD("\"%s\" starting", mSpec->name.c_str());

    UpdateParams p = mSpec->updateParams;
    const nsecs_t latency = p.latency;
    int visibility;
    bool positionChange, sizeChange;

    for (long int i = 0; (i < p.iterations || p.iterations < 0) && !exitPending(); i++) {
        if (latency > 0) {
            const nsecs_t sleepTime = latency - ns2us(systemTime() - mLastIter);
            if (sleepTime > 0)
                usleep (sleepTime);
            mLastIter = systemTime();
        }

#ifndef ADTF_ICS_AND_EARLIER
        if (mEventReceiver) {
            mEventLooper->pollOnce(-1);
            while (mEventReceiver->getEvents(mEventBuffer, 100) > 0);
            if (mEventReceiver->requestNextVsync() != NO_ERROR) {
                LOGE("\"%s\" failed to request vsync", mSpec->name.c_str());
                signalExit();
                return false;
            }
        }
#endif

        positionChange = updatePosition();
        sizeChange = updateSize();
        visibility = getVisibility();

        if (positionChange || sizeChange || (visibility != 0)) {
            SurfaceComposerClient::openGlobalTransaction();
            mStat.openTransaction();

            if (positionChange) {
                mSurfaceControl->setPosition(mLeft, mTop);
                mStat.setPosition();
            }

            if (sizeChange) {
                mSurfaceControl->setSize(mWidth, mHeight);
                mStat.setSize();
            }

            if (visibility < 0) {
                mSurfaceControl->hide();
                mStat.setVisibility();
            }
            else if (visibility > 0) {
                mSurfaceControl->show();
                mStat.setVisibility();
            }

            SurfaceComposerClient::closeGlobalTransaction();
            mStat.closeTransaction();
        }

        if (updateContent(sizeChange)) {
            mStat.startUpdate();
            updateContent();
            mStat.doneUpdate();
            mLastWidth = mWidth;
            mLastHeight = mHeight;
        }
        if (mStat.sinceClear() >= 1000000) {
            if (!mSpec->renderFlag(RenderFlags::SILENT))
                mStat.dump(mSpec->name);
            mStat.clear();
        }
    }

    mStat.dump(mSpec->name);

    LOGD("\"%s\" thread exiting", mSpec->name.c_str());

    signalExit();
    return false;
}
