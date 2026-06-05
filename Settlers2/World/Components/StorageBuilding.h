#ifndef WORLD_COMPONENTS_STORAGEBUILDING_H
#define WORLD_COMPONENTS_STORAGEBUILDING_H

#include "Building.h"

namespace World {

    class StorageBuilding : public Building {
    public:
        StorageBuilding(BuildingType t, int x, int y, uint8_t o, Map* m)
            : Building(t, x, y, o, m) {
                // Set default capacity for each resource type
                for (int i = 0; i < ResourceType_Count; ++i) {
                    m_capacity[i] = 100; // 100 units per resource by default
                }
            }

        virtual void Update() {
            // Storage buildings do not produce or consume by themselves
        }

        bool AddResource(ResourceType type, int amount) {
            if (type == ResourceType_None || amount <= 0) return false;
            if (m_storage[type] + amount > m_capacity[type])
                return false;
            m_storage[type] += amount;
            return true;
        }

        bool RemoveResource(ResourceType type, int amount) {
            if (type == ResourceType_None || amount <= 0) return false;
            if (m_storage[type] < amount)
                return false;
            m_storage[type] -= amount;
            return true;
        }

        int GetStorage(ResourceType type) const {
            if (type == ResourceType_None) return 0;
            return m_storage[type];
        }

        void SetCapacity(ResourceType type, int capacity) {
            if (type == ResourceType_None) return;
            if (capacity < 0) capacity = 0;
            m_capacity[type] = capacity;
        }

        int GetCapacity(ResourceType type) const {
            if (type == ResourceType_None) return 0;
            return m_capacity[type];
        }

    private:
        int m_capacity[ResourceType_Count];
    };

} // namespace World

#endif // WORLD_COMPONENTS_STORAGEBUILDING_H