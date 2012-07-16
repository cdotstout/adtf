/* San Angeles Observation OpenGL ES version example
 * Copyright 2009 The Android Open Source Project
 * All rights reserved.
 *
 * This source is free software; you can redistribute it and/or
 * modify it under the terms of EITHER:
 *   (1) The GNU Lesser General Public License as published by the Free
 *       Software Foundation; either version 2.1 of the License, or (at
 *       your option) any later version. The text of the GNU Lesser
 *       General Public License is included with this source in the
 *       file LICENSE-LGPL.txt.
 *   (2) The BSD-style license that is included with this source in
 *       the file LICENSE-BSD.txt.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the files
 * LICENSE-LGPL.txt and LICENSE-BSD.txt for more details.
 */
#include <sys/time.h>
#include <time.h>
#include <stdint.h>


static int  sWindowWidth  = 320;
static int  sWindowHeight = 480;
static long sTimeOffset   = 0;
static int  sTimeOffsetInit = 0;

static long
_getTime(void)
{
    struct timeval  now;

    gettimeofday(&now, NULL);
    return (long)(now.tv_sec*1000 + now.tv_usec/1000);
}

int create(int w, int h, int argc, char** argv, void** data)
{
    /* This demo uses global variables for state */
    *data = NULL;

    sWindowWidth = w;
    sWindowHeight = h;

    return 0;
}

int init(void* data)
{
    appInit();
    sTimeOffsetInit = 0;

    return 0;
}

int render(void* data)
{
    long curTime = _getTime() + sTimeOffset;
    if (sTimeOffsetInit == 0) {
        sTimeOffsetInit = 1;
        sTimeOffset     = -curTime;
        curTime         = 0;
    }

    appRender(curTime, sWindowWidth, sWindowHeight);

    return 0;
}

int sizeChanged (void* data, int w, int h)
{
    sWindowWidth  = w;
    sWindowHeight = h;

    return 0;
}

void destroy(void*data)
{
    appDeinit();
}
