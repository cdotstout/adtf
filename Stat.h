#ifndef _STAT_H
#define _STAT_H

#include <utils/Log.h>
#include <utils/Timers.h>

using namespace android;
using namespace std;

class Stat {
    public:
        Stat();
        void clear();
        nsecs_t sinceClear();
        void openTransaction();
        void closeTransaction();
        void startUpdate();
        void doneUpdate();
        void setPosition();
        void setSize();
        void setVisibility();
        void dump(string what);

    private:
        DurationTimer mClear;

        DurationTimer mTrans;
        nsecs_t mTransCount;
        nsecs_t mTransMin;
        nsecs_t mTransMax;
        nsecs_t mTransAvg;

        DurationTimer mUpdate;
        nsecs_t mUpdateCount;
        nsecs_t mUpdateMin;
        nsecs_t mUpdateMax;
        nsecs_t mUpdateAvg;

        nsecs_t mPosCount;
        nsecs_t mSizeCount;
        nsecs_t mVisCount;
};

#endif
