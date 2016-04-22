// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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

#include <iostream>
#include <vector>

#include <GLES/gl.h>

#include <gui/SurfaceComposerClient.h>
#include <gui/Surface.h>
#include <ui/PixelFormat.h>

#include <binder/ProcessState.h>

class Test {
public:
    Test() {}
    virtual ~Test() {
        purgeEglBuffers();
        freeEgl();
    }

    bool init();
    bool createSurface();
    bool initEgl();

    bool purgeEglBuffers();
    void freeEgl();

    void draw();
    void swap();

    void chooseEGLConfig(EGLDisplay display, EGLConfig *config, android::PixelFormat format);
    EGLContext createEGLContext(EGLDisplay display, EGLConfig config);

    android::sp<android::SurfaceComposerClient> mComposerClient;
    android::sp<android::SurfaceControl> mSurfaceControl;
    EGLDisplay mEglDisplay;
    EGLSurface mEglSurface;
    EGLContext mEglContext;
    int mWidth;
    int mHeight;
    int mLastWidth;
    int mLastHeight;
};

bool Test::init() {
    mComposerClient = new android::SurfaceComposerClient;

    fprintf(stderr, "Inc ref on composerClient\n");
    // Required to pass initCheck
    mComposerClient->incStrong(nullptr);

    android::status_t status;

    fprintf(stderr, "calling initCheck\n");

    status = mComposerClient->initCheck();
    if (status != android::NO_ERROR) {
        fprintf(stderr, "failed composer init check %d\n", status);
        return false;
    }

    return true;
}

bool Test::createSurface() {
    int w = 512;
    int h = 512;
    uint32_t flags = 0;

    mSurfaceControl = mComposerClient->createSurface(
        android::String8("adtf"),
        w,
        h,
        android::PIXEL_FORMAT_BGRA_8888,
        flags
    );

    fprintf(stderr, "Got SurfaceControl %p\n", mSurfaceControl->getSurface().get());
    
    android::SurfaceComposerClient::openGlobalTransaction();
    mSurfaceControl->setLayer(500001);
    mSurfaceControl->setPosition(0, 0);
    android::SurfaceComposerClient::closeGlobalTransaction();

    return true;
}


bool Test::initEgl()
{
    EGLConfig config = NULL;
    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(mEglDisplay, 0, 0);
    chooseEGLConfig(mEglDisplay, &config, android::PIXEL_FORMAT_BGRA_8888);
    if (config == NULL) {
        ALOGE("chooseEGLConfig failed");
        return false;
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
            ALOGD("EGLConfig %s : %d", attr_names[j], value);
        else
            while (eglGetError() != EGL_SUCCESS);
    }

    mEglSurface = eglCreateWindowSurface(mEglDisplay, config,
            mSurfaceControl->getSurface().get(), NULL);
    mEglContext = createEGLContext(mEglDisplay, config);
    if (mEglContext == 0) {
        ALOGE("createEGLContext failed");
        return false;
    }

    if (eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext) == EGL_FALSE) {
        ALOGE("eglMakeCurrent failed");
        return false;
    }

    return true;
}

bool Test::purgeEglBuffers()
{
    if (eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE) {
        ALOGE("eglMakeCurrent failed");
        return false;
    }

    if (eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext) == EGL_FALSE) {
        ALOGE("eglMakeCurrent failed");
        return false;
    }

    return true;
}

void Test::freeEgl()
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
void Test::chooseEGLConfig(EGLDisplay display, EGLConfig *config, android::PixelFormat format)
{
    int a, r, g, b;
    a = r = g = b = 0;
    *config = NULL;

    // Note: Non explicit formats are translated in SurfaceFlinger::createNormalSurface
    // If translation changes there, this must be updated. Use explicit formats to avoid problems
    switch (format) {
        // Translated to PIXEL_FORMAT_RGBA_8888
        case android::PIXEL_FORMAT_TRANSLUCENT:
        case android::PIXEL_FORMAT_TRANSPARENT:
        case android::PIXEL_FORMAT_RGBA_8888:
        case android::PIXEL_FORMAT_BGRA_8888:
            a = r = g = b = 8;
            break;

        case android::PIXEL_FORMAT_OPAQUE:
            // Warning: This one is board platform specific and should not be used!
            // Translated to PIXEL_FORMAT_RGBX_8888 for most
            ALOGW("PIXEL_FORMAT_OPAQUE used for EGL, treating as PIXEL_FORMAT_RGBX_8888");
        case android::PIXEL_FORMAT_RGBX_8888:
        case android::PIXEL_FORMAT_RGB_888:
            r = g = b = 8;
            a = 0;
        break;

        case android::PIXEL_FORMAT_RGB_565:
            r = b = 5;
            g = 6;
            a = 0;
            break;
        case android::PIXEL_FORMAT_RGBA_5551:
            r = g = b = 5;
            a = 1;
            break;
        case android::PIXEL_FORMAT_RGBA_4444:
            a = r = g = b = 4;
            break;

        case android::PIXEL_FORMAT_NONE:
        case android::PIXEL_FORMAT_CUSTOM:
        default:
            ALOGE("invalid format");
            return;
    }

    const EGLint attribs[] = {
        EGL_RED_SIZE,   r,
        EGL_GREEN_SIZE, g,
        EGL_BLUE_SIZE,  b,
        EGL_ALPHA_SIZE, a,
        EGL_NONE
    };

    ALOGD("calling eglChooseConfig with sizes r=%d g=%d b=%d a=%d",
            r, g, b, a);

    EGLint num;
    std::vector<EGLConfig> v;

    eglChooseConfig(display, attribs, NULL, 0, &num);
    v.reserve(num);
    eglChooseConfig(display, attribs, &v[0], num, &num);

    ALOGD("eglChooseConfig returned %d configs", num);

    // Take the first found config that matches exactly
    for (int i = 0; i < num; i++) {
        EGLint rv = -1, gv = -1, bv = -1, av = -1;
        eglGetConfigAttrib(mEglDisplay, v[i], EGL_RED_SIZE, &rv);
        eglGetConfigAttrib(mEglDisplay, v[i], EGL_GREEN_SIZE, &gv);
        eglGetConfigAttrib(mEglDisplay, v[i], EGL_BLUE_SIZE, &bv);
        eglGetConfigAttrib(mEglDisplay, v[i], EGL_ALPHA_SIZE, &av);
        if (rv == r && gv == g && bv == b && av == a) {
            ALOGD("using config %d with sizes r=%d g=%d b=%d a=%d",
                i, rv, gv, bv, av);
            memcpy(config, &v[i], sizeof(EGLConfig));
            return;
        }
    }

    // No exact match, warn and return first one
    ALOGW("no matching EGLConfig, using default");
    eglChooseConfig(display, attribs, config, 1, &num);
}

// Must be overridden for GLES 2
EGLContext Test::createEGLContext(EGLDisplay display, EGLConfig config)
{
    return eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
}

void Test::draw() {
    // Though a bit unintuitive, always interprete bytes as RBGA for gl for simplicity
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Test::swap() {
    eglSwapBuffers(mEglDisplay, mEglSurface);
}

int main (int argc, char** argv)
{
    // sp<ProcessState> proc(ProcessState::self());
    fprintf(stderr, "Starting thread pool\n");
    android::ProcessState::self()->startThreadPool();

    Test test;

    fprintf(stderr, "Initializing\n");
    if (!test.init())
        return -1;

    fprintf(stderr, "Creating a surface\n");
    if (!test.createSurface())
        return -1;

    fprintf(stderr, "Initializing EGL\n");
    if (!test.initEgl())
        return -1;

    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "Drawing\n");
        test.draw();
        test.swap();
        sleep(1);
    }

    fprintf(stderr, "Exiting\n");
    return 0;
}
