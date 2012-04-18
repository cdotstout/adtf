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

#include <iostream>

#include <binder/ProcessState.h>
#include <surfaceflinger/ISurfaceComposer.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
# include <sys/resource.h>
#endif

#include "ThreadManager.h"
#include "SpecParser.h"

#define VERSION "0.1.1"

using namespace android;
using namespace std;

void run(List<sp<SurfaceSpec> >& specs)
{
    sp<ThreadManager> mgr(new ThreadManager(specs));
    mgr->run();
    mgr->join(); // Won't return until all update threads have terminated
}

int main (int argc, char** argv)
{
    if (argc < 2) {
#ifdef VERSION
        cout << "Version " << VERSION << endl;
#endif
        cout << "Usage: " << argv[0] << " path_to_test_spec1 path_to_test_spec2 (...)" << endl;
        return -1;
    }

    LOGD(" ");

    List<sp<SurfaceSpec> > specs;
    for (int i = 1; i < argc; i++) {
        if (!SpecParser::parseFile(argv[i], specs)) {
            LOGW("parsing of '%s' failed", argv[i]);
            cout << "parsing of '" << argv[i] << "' failed" << endl;
        }
    }

    if (specs.size() == 0) {
        LOGE("invalid input, nothing to do");
        cout << "invalid input, nothing to do" << endl;
        return -1;
    }

#if defined(HAVE_PTHREADS)
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_DISPLAY);
#endif

    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    LOGD("running");
    run(specs);
    LOGD("done");

    return 0;
}
