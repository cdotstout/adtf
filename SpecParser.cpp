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
            int b = 0;
            ss >> b;
            if (ss.fail()) {
                string v;
                ss.clear();
                ss.str(line);
                ss >> prop >> v;
                if (v == "true")
                    spec->keepAlive = true;
                else if (v == "false")
                    spec->keepAlive = false;
                else
                    LOGW("%s:%u unknown %s '%s'", filename.c_str(), n, prop.c_str(), v.c_str());
            } else
                spec->keepAlive = b == 1;
            continue;
        } else if (prop == "format") {
            // If format is given as an int cast to PixelFormat, otherwise translate
            ss >> spec->format;
            if (ss.fail()) {
                // Try to translate string since it wasn't an int
                string pf;
                ss.clear();
                ss.str(line);
                ss >> prop >> pf;

                if (pf == "PIXEL_FORMAT_UNKNOWN")
                    spec->format = PIXEL_FORMAT_UNKNOWN;
                else if (pf == "PIXEL_FORMAT_NONE")
                    spec->format = PIXEL_FORMAT_NONE;
                else if (pf == "PIXEL_FORMAT_CUSTOM")
                    spec->format = PIXEL_FORMAT_CUSTOM;
                else if (pf == "PIXEL_FORMAT_TRANSLUCENT")
                    spec->format = PIXEL_FORMAT_TRANSLUCENT;
                else if (pf == "PIXEL_FORMAT_TRANSPARENT")
                    spec->format = PIXEL_FORMAT_TRANSPARENT;
                else if (pf == "PIXEL_FORMAT_OPAQUE")
                    spec->format = PIXEL_FORMAT_OPAQUE;
                else if (pf == "PIXEL_FORMAT_RGBA_8888")
                    spec->format = PIXEL_FORMAT_RGBA_8888;
                else if (pf == "PIXEL_FORMAT_RGBX_8888")
                    spec->format = PIXEL_FORMAT_RGBX_8888;
                else if (pf == "PIXEL_FORMAT_RGB_888")
                    spec->format = PIXEL_FORMAT_RGB_888;
                else if (pf == "PIXEL_FORMAT_RGB_565")
                    spec->format = PIXEL_FORMAT_RGB_565;
                else if (pf == "PIXEL_FORMAT_BGRA_8888")
                    spec->format = PIXEL_FORMAT_BGRA_8888;
                else if (pf == "PIXEL_FORMAT_RGBA_5551")
                    spec->format = PIXEL_FORMAT_RGBA_5551;
                else if (pf == "PIXEL_FORMAT_RGBA_4444")
                    spec->format = PIXEL_FORMAT_RGBA_4444;
                else if (pf == "PIXEL_FORMAT_A_8")
                    spec->format = PIXEL_FORMAT_A_8;
                else if (pf == "PIXEL_FORMAT_L_8")
                    spec->format = PIXEL_FORMAT_L_8;
                else if (pf == "PIXEL_FORMAT_LA_88")
                    spec->format = PIXEL_FORMAT_LA_88;
                else if (pf == "PIXEL_FORMAT_RGB_332")
                    spec->format = PIXEL_FORMAT_RGB_332;
                else
                    LOGW("%s:%u unknown %s '%s'", filename.c_str(), n, prop.c_str(), pf.c_str());
            }
            continue;        
        } else if (prop == "zorder") {
            ss >> spec->zOrder;
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