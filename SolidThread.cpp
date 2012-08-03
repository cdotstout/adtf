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
#include "SolidThread.h"

using namespace android;
using namespace std;

SolidThread::SolidThread(sp<SurfaceSpec> spec, sp<SurfaceComposerClient> client,
        Mutex &exitLock, Condition &exitCondition) :
    TestBase(spec, client, exitLock, exitCondition)
{
}

status_t SolidThread::readyToRun()
{
    string color;
    stringstream ss(stringstream::in | stringstream::out);
    ss.str(mSpec->content);

    mBpp = (mSpec->bufferFormat == HAL_PIXEL_FORMAT_TI_NV12) ? 2 :
           (mSpec->bufferFormat == HAL_PIXEL_FORMAT_TI_BGRX) ? 4 :
            bytesPerPixel(mSpec->format);
    if (mBpp > 4 || mBpp < 1) {
        LOGE("\"%s\" can't handle %d bytes per pixel", mSpec->name.c_str(), mBpp);
        signalExit();
        return UNKNOWN_ERROR;
    }

    mColors.clear();
    while (ss >> color) {
        if (color == "random") {
            mColors.push_back (ULONG_MAX + 1);
            continue;
        }

        unsigned long long v;
        stringstream ssv;
        ssv << hex << color;
        if (ssv.fail()) {
            LOGE("\"%s\" invalid color '%s'", mSpec->name.c_str(), color.c_str());
            signalExit();
            return UNKNOWN_ERROR;
        }
        ssv >> v;
        mColors.push_back (v);
    }

    if (mColors.size() == 0) {
        LOGW("\"%s\" no colors specified, using random", mSpec->name.c_str());
        mColors.push_back (ULONG_MAX + 1);
    }

    LOGD("\"%s\" got %d colors, bytes per pixel %d, gl %d",
            mSpec->name.c_str(), mColors.size(), mBpp, mSpec->renderFlag(RenderFlags::GL));

    createSurface();
    if (mSurfaceControl == 0 || done()) {
        LOGE("\"%s\" failed to create surface", mSpec->name.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    TestBase::initEgl();
    if (done()) {
        LOGE("\"%s\" initEgl failed", mSpec->name.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    return TestBase::readyToRun();
}

void SolidThread::updateContent()
{
    unsigned long long v = mColors.itemAt(0);
    mColors.removeAt(0);
    mColors.push_back(v);

    // Assuming no more than 4 bytes per pixel
    uint8_t b0, b1, b2, b3;

    if (v == (ULONG_MAX + 1)) {
        b0 = rand() % 255;
        b1 = rand() % 255;
        b2 = rand() % 255;
        b3 = rand() % 255;
    } else {
        v = v << ((4 - mBpp) * 8);
        b0 = (v & 0xFF000000) >> 24;
        b1 = (v & 0x00FF0000) >> 16;
        b2 = (v & 0x0000FF00) >> 8;
        b3 = (v & 0x000000FF);
    }

    sp<Surface> s = mSurfaceControl->getSurface();

    if (mSpec->renderFlag(RenderFlags::GL)) {
        // Though a bit unintuitive, always interprete bytes as RBGA for gl for simplicity
        glClearColor(b0 / 255.0, b1 / 255.0, b2 / 255.0, b3 / 255.0);
        glClear(GL_COLOR_BUFFER_BIT);
        eglSwapBuffers(mEglDisplay, mEglSurface);
        return;
    }

    if (mSpec->bufferFormat == HAL_PIXEL_FORMAT_TI_NV12) {
        GraphicBufferMapper &mapper = GraphicBufferMapper::get();
        sp<ANativeWindow> window(s);
        ANativeWindowBuffer *b;
        char *y, *uv;

        if (!TestBase::lockNV12(window, &b, &y, &uv)) {
            requestExit();
            return;
        }

        int i, strides = b->height;
        for (i = 0; i < strides; i++, y += b->stride)
            memset(y, b0, b->width);
        strides /= 2;
        for (i = 0; i < strides; i++, uv += b->stride)
            memset(uv, b1, b->width);

        mapper.unlock(b->handle);
        window.get()->queueBuffer(window.get(), b);
        return;
    }

    Surface::SurfaceInfo info;

    if (s->lock(&info) != NO_ERROR) {
        LOGE("\"%s\" failed to lock surface", mSpec->name.c_str());
        requestExit();
        return;
    }

    unsigned int sl = info.w * mBpp;
    char line [sl];
    for (uint32_t x = 0, i = 0; x < info.w; x++, i = x * mBpp) {
        switch (mBpp) {
            case 4:
                line[i + 3] = b3;
            case 3:
                line[i + 2] = b2;
            case 2:
                line[i + 1] = b1;
            case 1:
                line[i] = b0;
                break;
            default:
                break;
        }
    }

    char* dst = reinterpret_cast<char*>(info.bits);
    unsigned int dl = info.s * mBpp;
    for (unsigned int i = 0; i < info.h; i++, dst += dl)
        memcpy(dst, line, sl);
    s->unlockAndPost();
}
