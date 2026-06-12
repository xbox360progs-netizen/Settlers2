#pragma once
#include <vector>
#include <stdint.h>
#include "../Core/Vector2i.h"
#include "ObjectState.h"
#include "Handle.h"
#include "Flag.h"

namespace World {
    class Carrier;

    struct RoadData {
        uint32_t id;
        uint32_t flagAId;
        uint32_t flagBId;
        std::vector<Vector2i> tiles;
    };

    struct Road;
    typedef Handle<Road> RoadHandle;

    struct Road {
        uint32_t id;
        FlagHandle a, b;
        std::vector<Vector2i> tiles;
        Handle<Carrier> carrier;
        ObjectState state;

        Road() : id(0), state(Active) {}
    };
}
