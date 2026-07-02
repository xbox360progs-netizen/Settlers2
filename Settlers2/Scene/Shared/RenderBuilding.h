#pragma once
#include "RenderTransform.h"
#include "BuildingVisual.h"

namespace Scene {

// Composed DTO: spatial identity + visual identity.
struct RenderBuilding {
    RenderTransform transform;
    BuildingVisual  visual;
};

} // namespace Scene
