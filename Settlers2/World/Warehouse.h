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

        Warehouse(int x, int y, uint8_t o) : Building(Building_None, x, y, o, NULL) {
            for (int i = 0; i < ResourceType_Count; ++i) {
                resources[(ResourceType)i] = 0;
            }
        }

        virtual void Update() {
            // Pull one unit per frame from flag into warehouse storage
            if (connectedFlag) {
                for (int t = 0; t < ResourceType_Count; ++t) {
                    ResourceType type = (ResourceType)t;
                    if (type == ResourceType_None) continue;
                    if (connectedFlag->GetAvailable(type) > 0) {
                        connectedFlag->RemoveResource(type, 1);
                        AddResource(type, 1);
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
