#pragma once
#include <vector>
#include "../Core/Vector2i.h"
#include "ResourceNode.h"

namespace World {
    class Road;

    struct ResourceSlot {
        ResourceType type;
        int amount;
    };

    class Flag {
    public:
        Vector2i pos;
        ResourceSlot slots[8];
        std::vector<Road*> roads;
        
        // Список соседей для поиска пути (графа флагов)
        std::vector<Flag*> neighbors;

        Flag(int x, int y) {
            pos.x = x;
            pos.y = y;
            for (int i = 0; i < 8; ++i) {
                slots[i].type = ResourceType_None;
                slots[i].amount = 0;
            }
        }
    };
}
