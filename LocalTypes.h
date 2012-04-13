#ifndef _LOCAL_TYPES_H
#define _LOCAL_TYPES_H

#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <utils/RefBase.h>
#include <utils/List.h>
#include <string>

#define HAL_PIXEL_FORMAT_TI_NV12 0x100

namespace ContentType {
    enum Enum { SOLID, FILE, PLUGIN };
};

namespace RenderFlags {
    enum Enum {
        KEEPALIVE       = 1 << 0,
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
        DutyCycle showCycle;
        DutyCycle positionCycle;
        DutyCycle sizeCycle;
        android::Rect outRectStep;
        android::Rect outRectLimit;
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
        android::List<android::Rect> transparentRegionHint;
        ContentType::Enum contentType;
        std::string content;
        UpdateParams updateParams;
        uint32_t flags;

        SurfaceSpec() {
            // Set somewhat reasonable initial values in case user forgot
            // to specify them.
            name = "anonymous";
            renderFlags = 0;
            format = bufferFormat = android::PIXEL_FORMAT_NONE;
            zOrder = 0;
            transform = 0;
            srcGeometry.width = 100;
            srcGeometry.height = 100;
            srcGeometry.stride = srcGeometry.width;
            srcGeometry.crop.clear();
            outRect.clear();
            contentType = ContentType::SOLID;
            content = "FF0000FF";
            updateParams.iterations = 5;
            updateParams.latency = 1000000;
            updateParams.contentUpdateCycle.onCount = 1;
            updateParams.contentUpdateCycle.offCount = 0;
            updateParams.showCycle.onCount = 1;
            updateParams.showCycle.offCount = 0;
            updateParams.positionCycle.onCount = 1;
            updateParams.positionCycle.offCount = 0;
            updateParams.sizeCycle.onCount = 1;
            updateParams.sizeCycle.offCount = 0;
            updateParams.outRectStep.clear();
            updateParams.outRectLimit.clear();
            flags = 0;
        }

        bool renderFlag(RenderFlags::Enum f) {
            return (renderFlags & f) != 0;
        };
    private:
        SurfaceSpec& operator = (SurfaceSpec& li);
};

#endif
