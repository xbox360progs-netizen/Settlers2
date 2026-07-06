#pragma once

namespace Platform {

    // Counting semaphore for resource-limited thread synchronisation.
    class Semaphore {
    public:
        Semaphore(unsigned int initialCount, unsigned int maxCount);
        ~Semaphore();

        bool Acquire(unsigned int timeoutMs);  // false = timeout
        void Release(unsigned int count = 1);

    private:
        struct Impl;
        Impl* m_impl;

        Semaphore(const Semaphore&);
        Semaphore& operator=(const Semaphore&);
    };

} // namespace Platform