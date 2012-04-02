#ifndef _LOCAL_TYPES_H
#define _LOCAL_TYPES_H

#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <utils/RefBase.h>
#include <string>

#define HAL_PIXEL_FORMAT_TI_NV12 0x100

namespace ContentType {
    enum Enum { SOLID, FILE, PLUGIN };
};

namespace RenderFlags {
    enum Enum {
        KEEPALIVE      = 1 << 0,
        GL              = 1 << 1,
        ASYNC           = 1 << 2
    };
};

class DutyCycle {
    public:
        unsigned int onCount;
        unsigned int offCount;
};

class UpdateParams {
    public:
        long iterations;
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
        int renderFlags;
        android::PixelFormat format;
        android::PixelFormat bufferFormat;
        int zOrder;
        int transform;
        SrcGeometry srcGeometry;
        android::Rect outRect;
        ContentType::Enum contentType;
        std::string content;
        UpdateParams updateParams;
        uint32_t flags;

        bool renderFlag(RenderFlags::Enum f) {
            return (renderFlags & f) != 0;
        };
    private:
        SurfaceSpec& operator = (SurfaceSpec& li);
};

#endif
