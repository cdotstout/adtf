#define LOG_TAG "adtf"

#include <algorithm>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
       
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
    if (mData != 0)
        munmap(mData, mLength);

    if (mFd > 0)
        close(mFd);
}

status_t FileThread::readyToRun()
{
    struct stat sb;

    mFd = open(mSpec->content.c_str(), O_RDONLY);
    if (mFd < 0) {
        LOGE("\"%s\" can't open '%s'", mSpec->name.c_str(), mSpec->content.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    if (fstat(mFd, &sb) == -1) {
        LOGE("\"%s\" can't stat '%s'", mSpec->name.c_str(), mSpec->content.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    mLength = sb.st_size;
    mData = (char*)mmap(NULL, mLength, PROT_READ, MAP_PRIVATE, mFd, 0);
    if (mData == MAP_FAILED) {
        mData = 0;
        LOGE("\"%s\" mmap failed '%s'", mSpec->name.c_str(), mSpec->content.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    createSurface();

    if (mSurfaceControl == 0) {
        LOGE("\"%s\" failed to create surface", mSpec->name.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    // Find out how many frames the file contains. This relies on src geometry
    // and pixel format. If actual file size is not a multiple of one frame size
    // we'll bail out to avoid crashing and burning.

    if (mSpec->buffer_format == HAL_PIXEL_FORMAT_TI_NV12) {
        mFrameSize = mSpec->srcGeometry.stride * mSpec->srcGeometry.height * 3 / 2;
    } else {
        mBpp = bytesPerPixel(mSpec->format);
        mLineByLine = false;
        mFrameSize = mSpec->srcGeometry.stride * mSpec->srcGeometry.height * mBpp;

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

    if ((mLength % mFrameSize != 0)) {
            LOGE("\"%s\" '%s' doesn't contain an integer number of frames",
                        mSpec->name.c_str(), mSpec->content.c_str());
            signalExit();
            return UNKNOWN_ERROR;
    }

    mFrames = mLength / mFrameSize;
    mFrameIndex = 0;

    LOGD("\"%s\" opened '%s' with %d frames", mSpec->name.c_str(),
            mSpec->content.c_str(), mFrames);

    return TestBase::readyToRun();
}

void FileThread::updateContent()
{
    sp<Surface> s = mSurfaceControl->getSurface();

    if (mSpec->buffer_format == HAL_PIXEL_FORMAT_TI_NV12) {
        GraphicBufferMapper &mapper = GraphicBufferMapper::get();
        sp<ANativeWindow> window(s);
        ANativeWindowBuffer *b;
        void *y, *uv;

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
            char *dst = (char*)y;
            unsigned int b = min(sl, dl);
            for (unsigned int i = 0; i < h; i++, dst += dl, src += sl)
                memcpy(dst, src, b);
            dst = (char*)uv;
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
