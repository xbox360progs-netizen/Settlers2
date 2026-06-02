#pragma once
#include <cstdint>

namespace World {

struct MapNode
{
    uint16_t terrain;
    uint16_t height;

    uint8_t owner;
    uint8_t object;

    uint8_t resourceType;
    uint8_t resourceAmount;

    bool road;
    bool occupied;
};

} // namespace World
