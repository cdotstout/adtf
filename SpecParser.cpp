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

#include <ui/PixelFormat.h>
#include <utils/Log.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "SpecParser.h"

using namespace android;
using namespace std;

inline void trim(string& str)
{
    str.erase(0, str.find_first_not_of(' '));
    str.erase(str.find_last_not_of(' ') + 1);
}

inline Rect parseRect(stringstream& ss, string filename, unsigned int n,
        string prop, bool warnWH)
{
    int x, y, w, h;

    ss >> x >> y;
    if (!ss.fail()) {
        ss >> w >> h;
        if (ss.fail()) {
            if (warnWH)
                LOGW("%s:%u invalid w and/or h in %s, defaulting to 0", filename.c_str(), n, prop.c_str());
            w = h = 0;
        }
    } else {
        LOGW("%s:%u invalid x and/or y in %s, defaulting to 0", filename.c_str(), n, prop.c_str());
        x = y = 0;
    }

    return Rect(x, y, x + w, y + h);
}

inline bool parseBool(stringstream& ss, string line, string filename, unsigned int n,
        string prop)
{
    int b = 0;
    ss >> b;
    if (ss.fail()) {
        string v;
        ss.clear();
        ss.str(line);
        ss >> prop >> v;
        if (v == "true")
            return true;
        else if (v == "false")
            return false;
        else {
            LOGW("%s:%u unknown %s '%s'", filename.c_str(), n, prop.c_str(), v.c_str());
            return false;
        }
    } else
        return (b == 1);
}

inline PixelFormat parsePixelFormat(stringstream& ss, string line, string filename,
        unsigned int n, string prop)
{
    PixelFormat f = 0;

    // If format is given as an int cast to PixelFormat, otherwise translate
    ss >> f;
    if (ss.fail()) {
        // Try to translate string since it wasn't an int
        string pf;
        ss.clear();
        ss.str(line);
        ss >> prop >> pf;

        if (pf == "PIXEL_FORMAT_UNKNOWN")
            f = PIXEL_FORMAT_UNKNOWN;
        else if (pf == "PIXEL_FORMAT_NONE")
            f = PIXEL_FORMAT_NONE;
        else if (pf == "PIXEL_FORMAT_CUSTOM")
            f = PIXEL_FORMAT_CUSTOM;
        else if (pf == "PIXEL_FORMAT_TRANSLUCENT")
            f = PIXEL_FORMAT_TRANSLUCENT;
        else if (pf == "PIXEL_FORMAT_TRANSPARENT")
            f = PIXEL_FORMAT_TRANSPARENT;
        else if (pf == "PIXEL_FORMAT_OPAQUE")
            f = PIXEL_FORMAT_OPAQUE;
        else if (pf == "PIXEL_FORMAT_RGBA_8888")
            f = PIXEL_FORMAT_RGBA_8888;
        else if (pf == "PIXEL_FORMAT_RGBX_8888")
            f = PIXEL_FORMAT_RGBX_8888;
        else if (pf == "PIXEL_FORMAT_RGB_888")
            f = PIXEL_FORMAT_RGB_888;
        else if (pf == "PIXEL_FORMAT_RGB_565")
            f = PIXEL_FORMAT_RGB_565;
        else if (pf == "PIXEL_FORMAT_BGRA_8888")
            f = PIXEL_FORMAT_BGRA_8888;
        else if (pf == "PIXEL_FORMAT_RGBA_5551")
            f = PIXEL_FORMAT_RGBA_5551;
        else if (pf == "PIXEL_FORMAT_RGBA_4444")
            f = PIXEL_FORMAT_RGBA_4444;
        else if (pf == "PIXEL_FORMAT_A_8")
            f = PIXEL_FORMAT_A_8;
        else if (pf == "PIXEL_FORMAT_L_8")
            f = PIXEL_FORMAT_L_8;
        else if (pf == "PIXEL_FORMAT_LA_88")
            f = PIXEL_FORMAT_LA_88;
        else if (pf == "PIXEL_FORMAT_RGB_332")
            f = PIXEL_FORMAT_RGB_332;
        else if (pf == "HAL_PIXEL_FORMAT_TI_NV12")
            f = HAL_PIXEL_FORMAT_TI_NV12;
        else
            LOGW("%s:%u unknown %s '%s'", filename.c_str(), n, prop.c_str(), pf.c_str());
    }
    return f;
}

inline int parseRenderFlags(stringstream& ss, string line, string filename,
        unsigned int n, string prop)
{
    int f = 0, v = 0;
    string sv;
    stringstream ssv(stringstream::in | stringstream::out);

    while (ss >> sv) {
        v = 0;
        ssv.clear();
        ssv.str(sv);

        ssv >> v;
        if (ssv.fail()) {
            if (sv == "KEEPALIVE" || sv == "keepalive")
                v = RenderFlags::KEEPALIVE;
            else if (sv == "GL" || sv == "gl")
                v = RenderFlags::GL;
            else if (sv == "ASYNC" || sv == "async")
                v = RenderFlags::ASYNC;
            else if (sv == "SILENT" || sv == "silent")
                v = RenderFlags::SILENT;
            else
                LOGW("%s:%u unknown %s '%s'", filename.c_str(), n, prop.c_str(), sv.c_str());
        }

        f |= v;
        sv.clear();
    }
    return f;
}

inline int parseTransform(stringstream& ss, string line, string filename,
        unsigned int n, string prop)
{
    int t = 0, v = 0;
    string sv;
    stringstream ssv(stringstream::in | stringstream::out);

    while (ss >> sv) {
        v = 0;
        ssv.clear();
        ssv.str(sv);

        ssv >> v;
        if (ssv.fail()) {
            if (sv == "NATIVE_WINDOW_TRANSFORM_FLIP_H" || sv == "HAL_TRANSFORM_FLIP_H")
                v = HAL_TRANSFORM_FLIP_H;
            else if (sv == "NATIVE_WINDOW_TRANSFORM_FLIP_V" || sv == "HAL_TRANSFORM_FLIP_V")
                v = HAL_TRANSFORM_FLIP_V;
            else if (sv == "NATIVE_WINDOW_TRANSFORM_ROT_90" || sv == "HAL_TRANSFORM_ROT_90")
                v = HAL_TRANSFORM_ROT_90;
            else if (sv == "NATIVE_WINDOW_TRANSFORM_ROT_180" || sv == "HAL_TRANSFORM_ROT_180")
                v = HAL_TRANSFORM_ROT_180;
            else if (sv == "NATIVE_WINDOW_TRANSFORM_ROT_270" || sv == "HAL_TRANSFORM_ROT_270")
                v = HAL_TRANSFORM_ROT_270;
            else
                LOGW("%s:%u unknown %s '%s'", filename.c_str(), n, prop.c_str(), sv.c_str());
        }

        t |= v;
        sv.clear();
    }
    return t;
}

bool SpecParser::parseFile(string filename, List<sp<SurfaceSpec> >& specs)
{
    ifstream f(filename.c_str(), ifstream::in);
    if (!f) {
        LOGE("unable to open '%s' for reading", filename.c_str());
        return false;
    }

    sp<SurfaceSpec> spec;
    unsigned int n = 0;

    while (f.good()) {
        string line;

        getline(f, line);
        trim(line);
        n++;
        if (line.empty() || line.find_first_of('#') == 0)
            continue;

        if (spec == 0 && line != "surface") {
            LOGW("%s:%u ignoring '%s' before first 'surface' keyword", filename.c_str(), n, line.c_str());
            continue;
        }

        if (line == "surface") {
            if (spec != 0)
                specs.push_back(spec);
            spec = sp<SurfaceSpec>(new SurfaceSpec());
            continue;
        }

        string prop;
        stringstream ss(stringstream::in | stringstream::out);
        ss.str(line);
        ss >> prop;

        if (prop == "name") {
            if (line.size() < 5) {
                LOGW("%s:%u empty %s", filename.c_str(), n, prop.c_str());
                continue;
            }
            spec->name = line.substr(5);
        } else if (prop == "render_flags") {
            spec->renderFlags = parseRenderFlags(ss, line, filename, n, prop);
            continue;
        } else if (prop == "format") {
            spec->format = parsePixelFormat(ss, line, filename, n, prop);
            if (spec->bufferFormat == PIXEL_FORMAT_NONE)
                spec->bufferFormat = spec->format;
            continue;
        } else if (prop == "buffer_format") {
            spec->bufferFormat = parsePixelFormat(ss, line, filename, n, prop);
            if (spec->format == PIXEL_FORMAT_NONE)
                spec->format = spec->bufferFormat;
            continue;
        } else if (prop == "zorder") {
            ss >> spec->zOrder;
        } else if (prop == "transform") {
            spec->transform = parseTransform(ss, line, filename, n, prop);
            continue;
        } else if (prop == "width") {
            ss >> spec->srcGeometry.width;
            if (spec->srcGeometry.width > spec->srcGeometry.stride)
                spec->srcGeometry.stride = spec->srcGeometry.width;
        } else if (prop == "height") {
            ss >> spec->srcGeometry.height;
        } else if (prop == "stride") {
            ss >> spec->srcGeometry.stride;
            if (spec->srcGeometry.width > spec->srcGeometry.stride)
                spec->srcGeometry.width = spec->srcGeometry.stride;
        } else if (prop == "crop") {
            spec->srcGeometry.crop = parseRect(ss, filename, n, prop, false);
            continue;
        } else if (prop == "output") {
            spec->outRect = parseRect(ss, filename, n, prop, false);
            continue;
        } else if (prop == "transparent_hint") {
            Rect rect = parseRect(ss, filename, n, prop, false);
            spec->transparentRegionHint.push_back(rect);
            continue;
        } else if (prop == "contenttype") {
            string t;
            ss >> t;
            if (ss.fail()) {
                LOGW("%s:%u invalid %s", filename.c_str(), n, prop.c_str());
            } else {
                if (t == "solid" || t == "SOLID")
                    spec->contentType = ContentType::SOLID;
                else if (t == "file" || t == "FILE")
                    spec->contentType = ContentType::FILE;
                else if (t == "plugin" || t == "PLUGIN")
                    spec->contentType = ContentType::PLUGIN;
                else
                    LOGW("%s:%u invalid %s", filename.c_str(), n, prop.c_str());
            }
            continue;
        } else if (prop == "content") {
            if (line.size() < 8) {
                LOGW("'%s' invalid %s", filename.c_str(), prop.c_str());
                continue;
            }
            spec->content = line.substr(8);
        } else if (prop == "update_iterations") {
            ss >> spec->updateParams.iterations;
        } else if (prop == "update_latency") {
            ss >> spec->updateParams.latency;
        } else if (prop == "update_output_step") {
            spec->updateParams.outRectStep = parseRect(ss, filename, n, prop, true);
            continue;
        } else if (prop == "update_output_limit") {
            spec->updateParams.outRectLimit = parseRect(ss, filename, n, prop, true);
        } else if (prop == "update_content" || prop == "update_content_show" ||
                   prop == "update_content_position" || prop == "update_content_size") {
            DutyCycle *dc = NULL;
            if (prop == "update_content")
                dc = &spec->updateParams.contentUpdateCycle;
            else if (prop == "update_content_show")
                dc = &spec->updateParams.showCycle;
            else if (prop == "update_content_position")
                dc = &spec->updateParams.positionCycle;
            else if (prop == "update_content_size")
                dc = &spec->updateParams.sizeCycle;
            else
                continue; // Shouldn't happen
            dc->onCount = 1; dc->offCount = 0; // Default to every iteration
            ss >> dc->onCount >> dc->offCount;
        } else if (prop == "flags") {
            ss >> spec->flags;
        } else {

            // Check for deprecated properties
            if (prop == "update_content_on") {
                ss >> spec->updateParams.contentUpdateCycle.onCount;
            } else if (prop == "update_content_off") {
                ss >> spec->updateParams.contentUpdateCycle.offCount;
            } else if (prop == "update_content_show_on") {
                ss >> spec->updateParams.showCycle.onCount;
            } else if (prop == "update_content_show_off") {
                ss >> spec->updateParams.showCycle.offCount;
            } else {
                LOGW("'%s' ignored line '%s'", filename.c_str(), line.c_str());
            }
        }

        // Common error check for simple parameters
        if (ss.fail())
            LOGW("'%s' invalid %s", filename.c_str(), prop.c_str());
    }

    f.close();

    if (spec != 0)
        specs.push_back(spec);

    return true;
}
