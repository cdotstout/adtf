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
#include <sstream>

#include "LocalTypes.h"
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
