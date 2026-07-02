#pragma once
#include <stdint.h>

namespace Scene {

// Per-segment road preview DTO.
// A segment is either a single tile (worldX0==worldX1 && worldY0==worldY1)
// or a horizontal connection between two adjacent tiles (worldX0!=worldX1).
// Built by RoadPreviewPresentationSystem, projected to screen coords by
// ProjectionSystem, consumed by RoadPreviewPass.
struct RenderRoadSegment {
    float   worldX0;
    float   worldY0;
    float   worldX1;
    float   worldY1;
    int     screenX0;
    int     screenY0;
    int     screenX1;
    int     screenY1;
    bool    valid;          // true = path segment (white), false = neighbor hint (red)

    RenderRoadSegment()
        : worldX0(0), worldY0(0)
        , worldX1(0), worldY1(0)
        , screenX0(0), screenY0(0)
        , screenX1(0), screenY1(0)
        , valid(false) {}
};

} // namespace Scene
