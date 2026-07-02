#pragma once
#include "../Shared/RenderTransform.h"
#include "OverlayMarkerType.h"

namespace Scene {

struct RenderOverlayMarker {
    RenderTransform  transform;
    OverlayMarkerType markerType;
    uint8_t          resourceType;    // World::ResourceType for deposit markers, 0 otherwise

    RenderOverlayMarker()
        : markerType(OVERLAY_MARKER_MOUNTAIN)
        , resourceType(0) {}
};

} // namespace Scene
