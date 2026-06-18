#pragma once
#include <vector>
#include <stdint.h>
#include "Cargo.h"

namespace World {
    class CargoManager {
    public:
        CargoManager();

        Cargo* Allocate(ResourceType type, uint32_t amount, Handle<Flag> onFlag);
        void Release(uint32_t id);
        void ReleaseAllForFlag(Handle<Flag> flag);
        Cargo* GetById(uint32_t id) const;
        size_t GetCount() const { return m_cargo.size(); }
        Cargo* GetByIndex(size_t i) const;

        void Clear();

    private:
        std::vector<Cargo*> m_cargo;
        uint32_t m_nextId;

        Cargo* FindFreeSlot();
    };
}
