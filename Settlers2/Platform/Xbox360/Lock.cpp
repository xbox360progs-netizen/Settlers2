#include "../Lock.h"

// Xbox 360 uses CRITICAL_SECTION from the Xbox SDK (available via windows.h subset).
#include <windows.h>

struct Platform::Lock::Impl {
    CRITICAL_SECTION cs;
};

Platform::Lock::Lock() : m_impl(new Impl) {
    InitializeCriticalSection(&m_impl->cs);
}

Platform::Lock::~Lock() {
    DeleteCriticalSection(&m_impl->cs);
    delete m_impl;
}

void Platform::Lock::Acquire() {
    EnterCriticalSection(&m_impl->cs);
}

void Platform::Lock::Release() {
    LeaveCriticalSection(&m_impl->cs);
}