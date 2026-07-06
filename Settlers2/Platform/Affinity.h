#pragma once

namespace Platform {

    // Platform-independent thread affinity control.
    //
    // On Xbox 360: maps to XSetThreadProcessor (sets a single processor).
    // On Win32:    maps to SetThreadAffinityMask (may set multiple processors).
    //
    // Usage:
    //   Platform::Thread myThread;
    //   myThread.Start(WorkerProc, this);
    //   Platform::SetThreadAffinity(myThread.GetNativeHandle(), 1);  // core 1

    void SetThreadAffinity(void* nativeHandle, unsigned int processor);

} // namespace Platform