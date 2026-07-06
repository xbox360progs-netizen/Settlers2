#pragma once

namespace Platform {

    // Manual-reset or auto-reset event for thread signalling.
    class Event {
    public:
        Event(bool manualReset);
        ~Event();

        void Signal();
        void Reset();
        bool Wait(unsigned int timeoutMs);  // false = timeout, true = signalled

    private:
        struct Impl;
        Impl* m_impl;

        Event(const Event&);
        Event& operator=(const Event&);
    };

} // namespace Platform