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
        } else if (prop == "keepalive") {
            spec->keepAlive = parseBool(ss, line, filename, n, prop);
            continue;
        } else if (prop == "async") {
            spec->async = parseBool(ss, line, filename, n, prop);
            continue;
        } else if (prop == "format") {
            spec->format = parsePixelFormat(ss, line, filename, n, prop);
            if (spec->buffer_format == PIXEL_FORMAT_NONE)
                spec->buffer_format = spec->format;
            continue;
        } else if (prop == "buffer_format") {
            spec->buffer_format = parsePixelFormat(ss, line, filename, n, prop);
            if (spec->format == PIXEL_FORMAT_NONE)
                spec->format = spec->buffer_format;
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
        } else if (prop == "contenttype") {
            string t;
            ss >> t;
            if (ss.fail()) {
                LOGW("%s:%u invalid %s", filename.c_str(), n, prop.c_str());
            } else {
                if (t == "solid")
                    spec->contentType = SOLID_FILL;
                else if (t == "file")
                    spec->contentType = LOCAL_FILE;
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
        } else if (prop == "update_content_on") {
            ss >> spec->updateParams.contentUpdateCycle.onCount;
        } else if (prop == "update_content_off") {
            ss >> spec->updateParams.contentUpdateCycle.offCount;
        } else if (prop == "update_output_step") {
            spec->updateParams.outRectStep = parseRect(ss, filename, n, prop, true);
            continue;
        } else if (prop == "update_output_limit") {
            spec->updateParams.outRectLimit = parseRect(ss, filename, n, prop, true);
        } else if (prop == "update_content_show_on") {
            ss >> spec->updateParams.showCycle.onCount;
        } else if (prop == "update_content_show_off") {
            ss >> spec->updateParams.showCycle.offCount;
        } else if (prop == "flags") {
            ss >> spec->flags;
        } else {
            LOGW("'%s' ignored line '%s'", filename.c_str(), line.c_str());
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
