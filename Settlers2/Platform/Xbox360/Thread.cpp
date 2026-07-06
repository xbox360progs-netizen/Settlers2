#include <xtl.h>
#include "../Thread.h"

namespace Platform {

    struct Thread::Impl {
        HANDLE handle;
        bool running;

        Impl() : handle(NULL), running(false) {}
    };

    Thread::Thread()
        : m_impl(new Impl())
    {
    }

    Thread::~Thread()
    {
        if (m_impl->running) {
            Join();
        }
        delete m_impl;
        m_impl = NULL;
    }

    bool Thread::Start(ThreadEntry entry, void* param)
    {
        if (m_impl->running) return false;

        m_impl->handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)entry, param, 0, NULL);
        if (m_impl->handle == NULL) return false;

        m_impl->running = true;
        return true;
    }

    void Thread::Join()
    {
        if (!m_impl->running) return;
        WaitForSingleObject(m_impl->handle, INFINITE);
        CloseHandle(m_impl->handle);
        m_impl->handle = NULL;
        m_impl->running = false;
    }

    void* Thread::GetNativeHandle() const
    {
        return m_impl->handle;
    }

} // namespace Platform