#pragma once
#include "../Shared/RenderTransform.h"
#include "WildlifeVisual.h"

namespace Scene {

// Composed DTO: spatial identity + visual identity.
// No simulation pointers, no renderer state.
struct RenderWildlife {
    RenderTransform transform;
    WildlifeVisual  visual;
};

} // namespace Scene
