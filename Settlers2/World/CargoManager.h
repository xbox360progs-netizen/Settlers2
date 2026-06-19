#pragma once
#include <stdint.h>
#include "Cargo.h"

#define MAX_WORLD_CARGO 256

namespace World {
    class Flag;
    class FlagManager;
    class DemandManager;
    class StorehouseManager;

    class CargoManager {
    public:
        CargoManager();

        Cargo* Allocate(ResourceType type, uint32_t amount, Handle<Flag> onFlag);
        void Release(uint32_t id);
        void ReleaseAllForFlag(Handle<Flag> flag);
        Cargo* GetById(uint32_t id) const;

        void SetStorehouseManager(StorehouseManager* sm) { m_storehouseManager = sm; }

        // Active indices for O(1) iteration over live cargo
        int GetActiveCount() const { return m_activeCount; }
        Cargo* GetCargoByActiveIdx(int i) const { return const_cast<Cargo*>(&m_pool[m_activeIndices[i]]); }

        // Count cargo on a specific flag (for FLAG_MAX_CARGO checks)
        int CountCargoOnFlag(Handle<Flag> flag) const;

        // Pool-wide delivery check — call once per frame instead of per-flag CheckDeliveries
        void CheckDeliveries(DemandManager* dm, FlagManager* fm);

        int GetCount() const { return m_poolCount; }
        Cargo* GetByIndex(int i) const;

        void Clear();

    private:
        Cargo m_pool[MAX_WORLD_CARGO];
        uint32_t m_activeIndices[MAX_WORLD_CARGO];
        int m_activeCount;
        int m_poolCount;
        uint32_t m_nextId;
        StorehouseManager* m_storehouseManager;
    };
}
