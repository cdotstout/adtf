#ifndef _LOCAL_TYPES_H
#define _LOCAL_TYPES_H

#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <utils/RefBase.h>
#include <string>

enum ContentType { SOLID_FILL, LOCAL_FILE };

class DutyCycle {
    public:
        unsigned int onCount;
        unsigned int offCount;
};

class UpdateParams {
    public:
        unsigned int iterations;
        unsigned int latency;
        DutyCycle contentUpdateCycle;
        android::Rect srcCropStep;
        android::Rect outRectStep;
        android::Rect outRectLimit;
        DutyCycle showCycle;
};

class SrcGeometry {
    public:
        int width;
        int height;
        int stride;
        android::Rect crop;
};

class SurfaceSpec : public android::RefBase {
    public:
        std::string name;
        bool keepAlive;
        android::PixelFormat format;
        int zOrder;
        int orientation;
        SrcGeometry srcGeometry;
        android::Rect outRect;
        ContentType contentType;
        std::string content;
        UpdateParams updateParams;
        uint32_t flags;

    private:
        SurfaceSpec& operator = (SurfaceSpec& li);
};

#endif
