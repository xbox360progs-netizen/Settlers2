#pragma once
#include <vector>
#include <map>
#include "ResourceNode.h"
#include "Components/Building.h"
#include "Flag.h"

namespace World {
    class Worker; // Forward declaration

    class Warehouse : public Building {
    public:
        std::map<ResourceType, int> resources;
        std::vector<Worker*> specialists;

        Warehouse(int x, int y, uint8_t o) : Building(Storehouse, x, y, o, NULL) {
            for (int i = 0; i < ResourceType_Count; ++i) {
                resources[(ResourceType)i] = 0;
            }
        }

        virtual bool IsWarehouse() const { return true; }

        void Update(float dt) override {
            if (connectedFlag) {
                for (int si = 0; si < 8; ++si) {
                    ResourceSlot& slot = connectedFlag->slots[si];
                    if (slot.type == ResourceType_None || slot.amount <= 0) continue;
                    if (slot.destFlagId != 0) continue;
                    if (slot.amount - slot.reserved > 0) {
                        connectedFlag->RemoveResource(slot.type, 1);
                        AddResource(slot.type, 1);
                        break;
                    }
                }
            }
        }

        void AddResource(ResourceType type, int amount) {
            resources[type] += amount;
        }

        bool RemoveResource(ResourceType type, int amount) {
            if (resources[type] >= amount) {
                resources[type] -= amount;
                return true;
            }
            return false;
        }
    };
}
