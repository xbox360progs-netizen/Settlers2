#pragma once

// Platform/Timer.h — portable timing.
// Must be self-contained: includes the platform header that provides the APIs.

#if defined(_XBOX)
#include <xtl.h>
#else
#include <windows.h>
#endif

namespace Platform {

    inline unsigned int GetTickCount()
    {
        return ::GetTickCount();
    }

    inline bool QueryPerformanceCounter(__int64* value)
    {
        return ::QueryPerformanceCounter((LARGE_INTEGER*)value) != 0;
    }

    inline bool QueryPerformanceFrequency(__int64* frequency)
    {
        return ::QueryPerformanceFrequency((LARGE_INTEGER*)frequency) != 0;
    }

    inline void Sleep(unsigned int ms)
    {
        ::Sleep(ms);
    }

} // namespace Platform
