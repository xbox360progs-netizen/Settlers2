#pragma once
#include <vector>
#include "MapNode.h"

namespace World {
    class Flag; // Forward declaration

    class Road {
    public:
        Flag* start;
        Flag* end;
        std::vector<MapNode*> path;

        Road(Flag* s, Flag* e) : start(s), end(e) {}
    };
}
