#include <xtl.h>
#include "../Event.h"

namespace Platform {

    struct Event::Impl {
        HANDLE handle;

        Impl() : handle(NULL) {}
    };

    Event::Event(bool manualReset)
        : m_impl(new Impl())
    {
        m_impl->handle = CreateEvent(NULL, manualReset ? TRUE : FALSE, FALSE, NULL);
    }

    Event::~Event()
    {
        if (m_impl->handle) {
            CloseHandle(m_impl->handle);
        }
        delete m_impl;
        m_impl = NULL;
    }

    void Event::Signal()
    {
        SetEvent(m_impl->handle);
    }

    void Event::Reset()
    {
        ResetEvent(m_impl->handle);
    }

    bool Event::Wait(unsigned int timeoutMs)
    {
        DWORD result = WaitForSingleObject(m_impl->handle, timeoutMs);
        return result == WAIT_OBJECT_0;
    }

} // namespace Platform