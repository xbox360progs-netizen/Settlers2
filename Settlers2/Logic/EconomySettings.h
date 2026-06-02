#pragma once
#include <map>
#include "../World/ResourceNode.h"

namespace Logic {

    struct EconomySettings {
        std::map<World::ResourceType, uint32_t> priorities;

        EconomySettings() {
            // Default priorities
            for (int i = 0; i < World::ResourceType_Count; ++i) {
                priorities[(World::ResourceType)i] = 10; // Default priority
            }
            priorities[World::ResourceType_Coal] = 50;
            priorities[World::ResourceType_IronOre] = 40;
            priorities[World::ResourceType_Bread] = 60;
        }
    };
}
