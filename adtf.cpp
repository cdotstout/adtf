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
