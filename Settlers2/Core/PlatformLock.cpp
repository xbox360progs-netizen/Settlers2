#include "stdafx.h"
#include "PlatformLock.h"

// Xbox 360 implementation uses CRITICAL_SECTION.
struct PlatformLock::Impl {
    CRITICAL_SECTION cs;
};

PlatformLock::PlatformLock() : m_impl(new Impl) {
    InitializeCriticalSection(&m_impl->cs);
}

PlatformLock::~PlatformLock() {
    DeleteCriticalSection(&m_impl->cs);
    delete m_impl;
}

void PlatformLock::Lock() {
    EnterCriticalSection(&m_impl->cs);
}

void PlatformLock::Unlock() {
    LeaveCriticalSection(&m_impl->cs);
}
