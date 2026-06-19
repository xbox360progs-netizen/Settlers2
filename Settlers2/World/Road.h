#pragma once
#include <stdint.h>
#include "../Core/Vector2i.h"
#include "ObjectState.h"
#include "Handle.h"
#include "Flag.h"

#define MAX_ROAD_TILES 64

namespace World {
    class Carrier;

    struct RoadData {
        uint32_t id;
        uint32_t flagAId;
        uint32_t flagBId;
        Vector2i tiles[MAX_ROAD_TILES];
        uint32_t tileCount;
    };

    struct Road;
    typedef Handle<Road> RoadHandle;

    struct Road {
        uint32_t id;
        FlagHandle a, b;
        Vector2i tiles[MAX_ROAD_TILES];
        uint32_t tileCount;
        Handle<Carrier> carrier;
        ObjectState state;

        Road() : id(0), tileCount(0), state(Active) {}
    };
}
