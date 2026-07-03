#pragma once
#include "../Shared/RenderTransform.h"
#include "OverlayMarkerType.h"

namespace Scene {

struct RenderOverlayMarker {
    RenderTransform  transform;
    OverlayMarkerType markerType;
    uint8_t          resourceType;    // World::ResourceType for deposit markers, 0 otherwise
    uint16_t         amount;         // resource amount (e.g. deer count for hunting spots)

    RenderOverlayMarker()
        : markerType(OVERLAY_MARKER_MOUNTAIN)
        , resourceType(0)
        , amount(0) {}
};

} // namespace Scene
