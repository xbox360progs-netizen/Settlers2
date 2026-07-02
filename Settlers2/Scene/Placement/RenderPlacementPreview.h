#pragma once
#include "../Shared/RenderTransform.h"
#include "PlacementPreviewVisual.h"

namespace Scene {

// Composed DTO: spatial identity + visual identity.
// No simulation pointers, no renderer state.
struct RenderPlacementPreview {
    RenderTransform        transform;
    PlacementPreviewVisual visual;
};

} // namespace Scene
