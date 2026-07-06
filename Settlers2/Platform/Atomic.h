#pragma once

// Platform/Atomic.h — portable atomic operations using MSVC intrinsics.
// Must be self-contained: includes the platform header that provides the intrinsics.

#if defined(_XBOX)
#include <xtl.h>
#else
#include <windows.h>
#endif

namespace Platform {

    inline long AtomicIncrement(volatile long* value)
    {
        return InterlockedIncrement(value);
    }

    inline long AtomicDecrement(volatile long* value)
    {
        return InterlockedDecrement(value);
    }

    inline long AtomicExchange(volatile long* target, long value)
    {
        return InterlockedExchange(target, value);
    }

    inline long AtomicCompareExchange(volatile long* target, long exchange, long comparand)
    {
        return InterlockedCompareExchange(target, exchange, comparand);
    }

    inline long AtomicRead(volatile long* target)
    {
        return InterlockedExchangeAdd(target, 0L);
    }

    inline void MemoryFence()
    {
        MemoryBarrier();
    }

} // namespace Platform
