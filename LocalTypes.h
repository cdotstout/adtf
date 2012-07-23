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

#ifndef _LOCAL_TYPES_H
#define _LOCAL_TYPES_H

#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <utils/RefBase.h>
#include <utils/List.h>
#include <string>

#ifdef ADTF_ICS_AND_EARLIER
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/SurfaceComposerClient.h>
#include <surfaceflinger/ISurfaceComposer.h>
#else
#include <Surface.h>
#include <SurfaceComposerClient.h>
#include <ISurfaceComposer.h>
#include <gui/DisplayEventReceiver.h>
#include <utils/Looper.h>
#define LOGD ALOGD
#define LOGW ALOGW
#define LOGE ALOGE
#define LOGI ALOGI
#endif

#define HAL_PIXEL_FORMAT_TI_NV12 0x100

namespace ContentType {
    enum Enum { SOLID, FILE, PLUGIN };
};

namespace RenderFlags {
    enum Enum {
        KEEPALIVE       = 1 << 0,
        GL              = 1 << 1,
        ASYNC           = 1 << 2,
        SILENT          = 1 << 3,
        VSYNC           = 1 << 4,
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
