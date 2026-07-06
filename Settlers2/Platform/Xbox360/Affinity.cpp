#include <xtl.h>
#include "../Affinity.h"

namespace Platform {

    void SetThreadAffinity(void* nativeHandle, unsigned int processor)
    {
        XSetThreadProcessor((HANDLE)nativeHandle, processor);
    }

} // namespace Platform