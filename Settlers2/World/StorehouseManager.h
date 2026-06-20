#pragma once
#include <stdint.h>
#include <string.h>
#include "../World/ResourceNode.h"

namespace World {

    struct StorehouseData {
        uint32_t resources[ResourceType_Count];
    };

    static const int MAX_STOREHOUSES = 32;

    class StorehouseManager {
    public:
        StorehouseManager();
        ~StorehouseManager();

        void Init();
        void Clear();

        int RegisterStorehouse();
        void UnregisterStorehouse(int handleIndex);
        int GetActiveCount() const { return m_activeCount; }
        int GetStorehouseIndex(int i) const { return m_activeIndices[i]; }

        StorehouseData* GetStorehouse(int handleIndex);

        void AddResourceToStorehouse(int handleIndex, ResourceType type, uint32_t amount);
        bool RemoveResourceFromStorehouse(int handleIndex, ResourceType type, uint32_t amount);
        void ModifyTransitResource(ResourceType type, int delta);

        uint32_t GetStoredCount(ResourceType type) const { return m_globalStored[type]; }
        uint32_t GetInTransitCount(ResourceType type) const { return m_globalInTransit[type]; }
        uint32_t GetTotalAvailable(ResourceType type) const { return m_globalStored[type] + m_globalInTransit[type]; }

    private:
        StorehouseData* m_storehouses;
        int m_activeIndices[MAX_STOREHOUSES];
        int m_activeCount;

        uint32_t m_globalStored[ResourceType_Count];
        uint32_t m_globalInTransit[ResourceType_Count];
    };

}
