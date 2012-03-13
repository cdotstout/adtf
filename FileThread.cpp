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

    Surface::SurfaceInfo info;
    sp<Surface> s = mSurfaceControl->getSurface();
    if (s->lock(&info) != NO_ERROR) { // only to get SurfaceInfo
        LOGE("\"%s\" failed to lock surface", mSpec->name.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }
    s->unlockAndPost();

    mBpp = bytesPerPixel(mSpec->format);
    mLineByLine = false;
    mFrameSize = mSpec->srcGeometry.stride * mSpec->srcGeometry.height * mBpp;

    if ((mLength % mFrameSize != 0)) {
        LOGE("\"%s\" '%s' doesn't contain an integer number of frames",
                    mSpec->name.c_str(), mSpec->content.c_str());
        signalExit();
        return UNKNOWN_ERROR;
    }

    // Do we have scaling going on, or is input stride wrong?
    mLineByLine = info.s != (uint32_t)mSpec->srcGeometry.stride ||
            info.h != (uint32_t)mSpec->srcGeometry.height;

    mFrames = mLength / mFrameSize;
    mFrameIndex = 0;

    LOGD("\"%s\" opened '%s' with %d frames, line-by-line %d", mSpec->name.c_str(),
            mSpec->content.c_str(), mFrames, mLineByLine);

    return TestBase::readyToRun();
}

void FileThread::updateContent()
{
    Surface::SurfaceInfo info;
    sp<Surface> s = mSurfaceControl->getSurface();
    if (s->lock(&info) != NO_ERROR) { // only to get SurfaceInfo
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
        // Simple and wished for case
       memcpy(dst, mData + mFrameIndex * mFrameSize, mFrameSize);
    }

    s->unlockAndPost();

    mFrameIndex = (mFrameIndex + 1) % mFrames;
}
