#include <string>
#include <utils/List.h>

#include "LocalTypes.h"

using namespace android;

class SpecParser
{
    public:
        static bool parseFile (std::string filename, List<sp<SurfaceSpec> >& specs);
};
