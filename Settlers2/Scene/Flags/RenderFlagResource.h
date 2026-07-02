#pragma once
#include <stdint.h>

namespace Scene {

// Per-frame flag resource icon DTO.
// Built by FlagResourcePresentationSystem (world coords + resource type),
// projected to screen coords by ProjectionSystem,
// consumed by FlagResourcePass.
struct RenderFlagResource {
    float  worldX;
    float  worldY;
    int    screenX;
    int    screenY;
    uint8_t resourceType;   // World::ResourceType enum value
    int8_t  stackOrder;     // stacking position (0 = top), not necessarily == slot index
    int     tileY;          // flag tile Y for depth computation

    RenderFlagResource()
        : worldX(0), worldY(0)
        , screenX(0), screenY(0)
        , resourceType(0)
        , stackOrder(0)
        , tileY(0)
    {}
};

} // namespace Scene
