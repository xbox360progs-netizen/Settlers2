#pragma once
#include <stdint.h>
#include "../Shared/RenderTransform.h"
#include "../../World/ResourceNode.h"

namespace Scene {

struct RenderGroundResource {
    RenderTransform transform;
    World::ResourceType resourceType;
    int   amount;
    bool  visualOnly;
    int   textScreenX;      // pre-projected text position (worldY - 40)
    int   textScreenY;
    uint8_t padding[3];

    RenderGroundResource() : resourceType(World::ResourceType_None), amount(0),
        visualOnly(false), textScreenX(0), textScreenY(0) { padding[0]=0;padding[1]=0;padding[2]=0; }
};

} // namespace Scene
