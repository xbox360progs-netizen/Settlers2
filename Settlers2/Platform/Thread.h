#pragma once

namespace Platform {

    // Thread entry point signature (__stdcall matches CreateThread on both Win32 and Xbox).
    typedef unsigned int (__stdcall* ThreadEntry)(void* param);

    // A single OS thread. PIMPL hides platform-specific handle type (HANDLE).
    class Thread {
    public:
        Thread();
        ~Thread();

        // Start executing `entry` with `param`. Returns false on failure.
        bool Start(ThreadEntry entry, void* param);

        // Wait for thread to exit.
        void Join();

        // OS handle — used by Affinity::Set() and other platform queries.
        void* GetNativeHandle() const;

    private:
        struct Impl;
        Impl* m_impl;

        // noncopyable
        Thread(const Thread&);
        Thread& operator=(const Thread&);
    };

} // namespace Platform