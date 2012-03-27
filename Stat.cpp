#define LOG_TAG "adtf"

#include <algorithm>
#include <sstream>

#include "Stat.h"

using namespace android;
using namespace std;

Stat::Stat()
{
    clear();
}

void Stat::clear()
{
    mTransCount = 0;
    mTransMin = LONGLONG_MAX;
    mTransMax = 0;;
    mTransAvg = 0;

    mUpdateCount = 0;
    mUpdateMin = LONGLONG_MAX;
    mUpdateMax = 0;
    mUpdateAvg = 0;

    mPosCount = 0;
    mSizeCount = 0;
    mVisCount = 0;

    mClear.start();
}

nsecs_t Stat::sinceClear()
{
    mClear.stop();
    return (nsecs_t)mClear.durationUsecs();
}

void Stat::openTransaction()
{
    mTrans.start();
}

void Stat::closeTransaction()
{
    mTrans.stop();
    mTransCount++;

    nsecs_t duration = mTrans.durationUsecs();
    mTransAvg = mTransAvg + (duration - mTransAvg) / mTransCount;
    mTransMin = min(mTransMin, duration);
    mTransMax = max(mTransMax, duration);
    
}

void Stat::startUpdate()
{
    mUpdate.start();
}

void Stat::doneUpdate()
{
    mUpdate.stop();
    mUpdateCount++;

    nsecs_t duration = mUpdate.durationUsecs();
    mUpdateAvg = mUpdateAvg + (duration - mUpdateAvg) / mUpdateCount;
    mUpdateMin = min(mUpdateMin, duration);
    mUpdateMax = max(mUpdateMax, duration);
}

void Stat::setPosition()
{
    mPosCount++;
}

void Stat::setSize()
{
    mSizeCount++;
}

void Stat::setVisibility()
{
    mVisCount++;
}

void Stat::dump(string what)
{
    stringstream ss;
    nsecs_t count, avg, min, max;

    count = mTransCount;
    avg = mTransAvg;
    min = mTransMin;
    max = mTransMax;    
    if (count <= 0)
        avg = min = max = 0;
    ss << " t: " << count << "/" << avg << "/" << min << "/" << max;

    count = mUpdateCount;
    avg = mUpdateAvg;
    min = mUpdateMin;
    max = mUpdateMax;

    if (count <= 0)
        avg = min = max = 0;

    ss << " u: " << count << "/" << avg << "/" << min << "/" << max;

    ss << " p: " << mPosCount;
    ss << " s: " << mSizeCount;
    ss << " v: " << mVisCount;
    ss << " d: " << sinceClear();

    LOGI("stat \"%s\"%s", what.c_str(), ss.str().c_str());
}
