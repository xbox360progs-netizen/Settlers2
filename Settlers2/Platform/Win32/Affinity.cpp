#include <windows.h>
#include "../Affinity.h"

namespace Platform {

    void SetThreadAffinity(void* nativeHandle, unsigned int processor)
    {
        HANDLE hThread = (HANDLE)nativeHandle;
        SetThreadAffinityMask(hThread, (DWORD_PTR)1 << processor);
    }

} // namespace Platform