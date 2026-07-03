#pragma once

class PlatformLock {
public:
    PlatformLock();
    ~PlatformLock();
    void Lock();
    void Unlock();
private:
    PlatformLock(const PlatformLock&);
    PlatformLock& operator=(const PlatformLock&);
    struct Impl;
    Impl* m_impl;
};
