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
    uint8_t padding[3];
};

} // namespace Scene
