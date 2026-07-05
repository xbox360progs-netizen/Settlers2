#pragma once

namespace Platform {

    class Lock {
    public:
        Lock();
        ~Lock();
        void Acquire();
        void Release();

    private:
        Lock(const Lock&);
        Lock& operator=(const Lock&);

        struct Impl;
        Impl* m_impl;
    };

} // namespace Platform