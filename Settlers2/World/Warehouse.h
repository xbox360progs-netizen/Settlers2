#pragma once
#include <vector>
#include "ResourceNode.h"
#include "Components/Building.h"
#include "Flag.h"
#include "StorehouseManager.h"

namespace World {
    struct Worker; // Forward declaration

    class Warehouse : public Building {
    public:
        std::vector<Worker*> specialists;
        int m_storehouseIndex;
        StorehouseManager* m_storehouseManager;

        Warehouse(int x, int y, uint8_t o)
            : Building(Storehouse, x, y, o, NULL)
            , m_storehouseIndex(-1)
            , m_storehouseManager(NULL)
        {
        }

        void SetStorehouseManager(StorehouseManager* sm) {
            m_storehouseManager = sm;
            if (sm) {
                m_storehouseIndex = sm->RegisterStorehouse();
            }
        }

        virtual bool IsWarehouse() const { return true; }

        void Update(float dt) override {
            if (connectedFlag && m_storehouseManager && m_storehouseIndex >= 0) {
                for (int si = 0; si < 8; ++si) {
                    ResourceSlot& slot = connectedFlag->slots[si];
                    if (slot.type == ResourceType_None || slot.amount <= 0) continue;
                    if (slot.destFlagId != 0 && slot.destFlagId != World::INVALID_FLAG_ID) {
                        if (slot.destFlagId != connectedFlag->id) continue;
                    }
                    if (slot.amount > 0) {
                        connectedFlag->RemoveResource(slot.type, 1);
                        AddResource(slot.type, 1);
                        break;
                    }
                }
            }
        }

        void AddResource(ResourceType type, int amount) {
            if (m_storehouseManager && m_storehouseIndex >= 0) {
                m_storehouseManager->AddResourceToStorehouse(m_storehouseIndex, type, (uint32_t)amount);
            }
        }

        bool RemoveResource(ResourceType type, int amount) {
            if (m_storehouseManager && m_storehouseIndex >= 0) {
                return m_storehouseManager->RemoveResourceFromStorehouse(m_storehouseIndex, type, (uint32_t)amount);
            }
            return false;
        }
    };
}
