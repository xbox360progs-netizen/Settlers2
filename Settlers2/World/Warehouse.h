#pragma once
#include <vector>
#include <map>
#include "ResourceNode.h"
#include "Components/Building.h"

namespace World {
    class Worker; // Forward declaration

    class Warehouse : public Building {
    public:
        std::map<ResourceType, int> resources;
        std::vector<Worker*> specialists;

        Warehouse(int x, int y, uint8_t o) : Building(Building_None, x, y, o, NULL) {
            // Initialize resources to 0
            for (int i = 0; i < ResourceType_Count; ++i) {
                resources[(ResourceType)i] = 0;
            }
        }

        virtual void Update() {
            // Warehouse doesn't "produce" anything itself, it just holds resources
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
