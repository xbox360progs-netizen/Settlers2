#include <windows.h>
#include "../Semaphore.h"

namespace Platform {

    struct Semaphore::Impl {
        HANDLE handle;

        Impl() : handle(NULL) {}
    };

    Semaphore::Semaphore(unsigned int initialCount, unsigned int maxCount)
        : m_impl(new Impl())
    {
        m_impl->handle = CreateSemaphore(NULL, initialCount, maxCount, NULL);
    }

    Semaphore::~Semaphore()
    {
        if (m_impl->handle) {
            CloseHandle(m_impl->handle);
        }
        delete m_impl;
        m_impl = NULL;
    }

    bool Semaphore::Acquire(unsigned int timeoutMs)
    {
        DWORD result = WaitForSingleObject(m_impl->handle, timeoutMs);
        return result == WAIT_OBJECT_0;
    }

    void Semaphore::Release(unsigned int count)
    {
        ReleaseSemaphore(m_impl->handle, count, NULL);
    }

} // namespace Platform