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

#include <algorithm>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>

#include "FileThread.h"

using namespace android;
using namespace std;

FileThread::FileThread(sp<SurfaceSpec> spec, sp<SurfaceComposerClient> client,
        Mutex &exitLock, Condition &exitCondition) :
    TestBase(spec, client, exitLock, exitCondition), mFd(-1), mData(0),
    mLength(0), mFrameSize(0), mFrames(0), mFrameIndex(0), mLineByLine(false)
{
}

FileThread::~FileThread()
{
    for (vector<GLuint>::iterator it = mTIds.begin(); it < mTIds.end(); it++ )
        glDeleteTextures(1, it);

    if (mData != 0)
        munmap(mData, mLength);

    if (mFd > 0)
        close(mFd);
}

status_t FileThread::readyToRun()
{
    struct stat sb;

    size_t frames = -1;
    string fileName;
    stringstream ss(stringstream::in | stringstream::out);
    ss.str(mSpec->content);
    ss >> fileName;
    ss >> frames;

    mFd = open(fileName.c_str(), O_RDONLY);
    if (mFd < 0) {
        LOGE("\"%s\" can't open '%s'", mSpec->name.c_str(), fileName.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    if (fstat(mFd, &sb) == -1) {
        LOGE("\"%s\" can't stat '%s'", mSpec->name.c_str(), fileName.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    mLength = sb.st_size;
    mData = (char*)mmap(NULL, mLength, PROT_READ, MAP_PRIVATE, mFd, 0);
    if (mData == MAP_FAILED) {
        mData = 0;
        LOGE("\"%s\" mmap failed '%s'", mSpec->name.c_str(), fileName.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    createSurface();
    if (mSurfaceControl == 0 || done()) {
        LOGE("\"%s\" failed to create surface", mSpec->name.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    // Find out how many frames the file contains. This relies on src geometry
    // and pixel format. If actual file size is not a multiple of one frame size
    // we'll bail out to avoid crashing and burning.

    if (mSpec->bufferFormat == HAL_PIXEL_FORMAT_TI_NV12) {
        mFrameSize = mSpec->srcGeometry.stride * mSpec->srcGeometry.height * 3 / 2;
    } else {
        mBpp = bytesPerPixel(mSpec->bufferFormat);
        mLineByLine = false;
        mFrameSize = mSpec->srcGeometry.stride * mSpec->srcGeometry.height * mBpp;

        if (!mSpec->renderFlag(RenderFlags::GL)) {
            Surface::SurfaceInfo info;
            sp<Surface> s = mSurfaceControl->getSurface();
            if (s->lock(&info) != NO_ERROR) { // only to get SurfaceInfo
                LOGE("\"%s\" failed to lock surface", mSpec->name.c_str());
                signalExit();
                return UNKNOWN_ERROR;
            }
            s->unlockAndPost();
            mLineByLine = info.s != (uint32_t)mSpec->srcGeometry.stride ||
                    info.h != (uint32_t)mSpec->srcGeometry.height;
        }
    }

    if ((mLength % mFrameSize != 0)) {
        if (mSpec->bufferFormat == HAL_PIXEL_FORMAT_TI_NV12) {
            LOGE("\"%s\" '%s' doesn't contain an integer number of frames (%dx%dx3/2=%d, file %d)",
                mSpec->name.c_str(), fileName.c_str(), mSpec->srcGeometry.stride,
                        mSpec->srcGeometry.height, mFrameSize, mLength);
        } else {
            LOGE("\"%s\" '%s' doesn't contain an integer number of frames (%dx%dx%d=%d, file %d)",
                mSpec->name.c_str(), fileName.c_str(), mSpec->srcGeometry.stride,
                        mSpec->srcGeometry.height, mBpp, mFrameSize, mLength);
        }
        signalExit();
        return UNKNOWN_ERROR;
    }

    mFrames = mLength / mFrameSize;
    mFrameIndex = 0;

    LOGD("\"%s\" opened '%s' with %d frames, using %d, gl %d", mSpec->name.c_str(),
            fileName.c_str(), mFrames, (frames > 0 ? min(frames,mFrames) : mFrames),
            mSpec->renderFlag(RenderFlags::GL));
    mFrames = frames > 0 ? min(frames,mFrames) : mFrames;

    if (mSpec->renderFlag(RenderFlags::GL)) {
        glShadeModel(GL_FLAT);
        glDisable(GL_DITHER);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        eglSwapBuffers(mEglDisplay, mEglSurface);

        glBindTexture(GL_TEXTURE_2D, 0);
        glEnable(GL_TEXTURE_2D);
        glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        for (size_t i = 0; i < mFrames; i++) {
            GLuint tid;
            glGenTextures(1, &tid);
            glBindTexture(GL_TEXTURE_2D, tid);
            glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            if (!initTexture(mData + i * mFrameSize)) {
                signalExit();
                return UNKNOWN_ERROR;
            }
            mTIds.push_back(tid);
        }
    }
    return TestBase::readyToRun();
}

bool FileThread::initTexture(void* p)
{
    bool ret = true;
    const int w = mSpec->srcGeometry.width;
    const int h = mSpec->srcGeometry.height;
    GLint crop[4] = { 0, h, w, -h };

    int tw = 1 << (31 - __builtin_clz(w));
    int th = 1 << (31 - __builtin_clz(h));
    if (tw < w) tw <<= 1;
    if (th < h) th <<= 1;

    switch (mSpec->bufferFormat) {
        // Not entierly right, but we care more about rendering something than nothing
        case PIXEL_FORMAT_RGBA_8888:
        case PIXEL_FORMAT_BGRA_8888:
            if (tw != w || th != h) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA,
                        GL_UNSIGNED_BYTE, 0);
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                        0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, p);
            } else {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA,
                        GL_UNSIGNED_BYTE, p);
            }
            break;
        case PIXEL_FORMAT_RGB_565:
            if (tw != w || th != h) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB,
                        GL_UNSIGNED_SHORT_5_6_5, 0);
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                        0, 0, w, h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, p);
            } else {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB,
                        GL_UNSIGNED_SHORT_5_6_5, p);
            }
            break;
        case HAL_PIXEL_FORMAT_TI_NV12:
        default:
            LOGE("\"%s\" unsupported texture format", mSpec->name.c_str());
            ret = false;
            break;
    }

    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, crop);

    return ret;
}

void FileThread::updateContent()
{
    sp<Surface> s = mSurfaceControl->getSurface();

    if (mSpec->renderFlag(RenderFlags::GL)) {
        if (mWidth != mLastWidth || mHeight != mLastHeight) {
            // Render once with the old dimensions
            glBindTexture(GL_TEXTURE_2D, mTIds.at(mFrameIndex));
            glDrawTexiOES(0, 0, 0, mLastWidth, mLastHeight);
            eglSwapBuffers(mEglDisplay, mEglSurface);

            // Purge buffers
            if (!TestBase::purgeEglBuffers()) {
                requestExit();
                return;
            }

            glViewport(0, 0, mWidth, mHeight);
        }
        glBindTexture(GL_TEXTURE_2D, mTIds.at(mFrameIndex));
        glDrawTexiOES(0, 0, 0, mWidth, mHeight);
        eglSwapBuffers(mEglDisplay, mEglSurface);
    } else if (mSpec->bufferFormat == HAL_PIXEL_FORMAT_TI_NV12) {
        GraphicBufferMapper &mapper = GraphicBufferMapper::get();
        sp<ANativeWindow> window(s);
        ANativeWindowBuffer *b;
        char *y, *uv;

        if (!TestBase::lockNV12(window, &b, &y, &uv)) {
            requestExit();
            return;
        }

        if (b->stride == mSpec->srcGeometry.stride &&
                b->height == mSpec->srcGeometry.height) {
            char *src = mData + mFrameIndex * b->stride * b->height * 3 / 2;
            int len = b->stride * b->height;
            memcpy(y, src, len);
            memcpy(uv, src + len, len / 2);
        } else {
            // Copy line by line
            unsigned int sl = mSpec->srcGeometry.stride, dl = b->stride;
            unsigned int h = min(mSpec->srcGeometry.height, b->height);
            char *src = mData + mFrameIndex * sl * mSpec->srcGeometry.height * 3 / 2;
            char *dst = y;
            unsigned int b = min(sl, dl);
            for (unsigned int i = 0; i < h; i++, dst += dl, src += sl)
                memcpy(dst, src, b);
            dst = uv;
            for (unsigned int i = 0; i < h / 2; i++, dst += dl, src += sl)
                memcpy(dst, src, b);
        }

        mapper.unlock(b->handle);
        window.get()->queueBuffer(window.get(), b);
    } else {
        Surface::SurfaceInfo info;
        if (s->lock(&info) != NO_ERROR) {
            LOGE("\"%s\" failed to lock surface", mSpec->name.c_str());
            requestExit();
            return;
        }

        char* dst = reinterpret_cast<char*>(info.bits);
        if (mLineByLine) {
            unsigned int sl = mSpec->srcGeometry.stride * mBpp, dl = info.s * mBpp;
            unsigned int h = min((unsigned int)mSpec->srcGeometry.height, info.h);
            char* src = mData + mFrameIndex * sl * mSpec->srcGeometry.height;
            unsigned int b = min(sl, dl);
            for (unsigned int i = 0; i < h; i++, dst += dl, src += sl)
                memcpy(dst, src, b);
        } else {
           memcpy(dst, mData + mFrameIndex * mFrameSize, mFrameSize);
        }

        s->unlockAndPost();
    }

    mFrameIndex = (mFrameIndex + 1) % mFrames;
}
