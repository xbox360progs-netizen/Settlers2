#pragma once
#include <stdint.h>

namespace Scene {

// Per-segment committed road connection DTO.
// A connection quad between two adjacent horizontal road tiles.
// Built by RoadConnectionPresentationSystem, projected by ProjectionSystem,
// consumed by RoadConnectionPass.
struct RenderRoadConnection {
    float   worldX0;
    float   worldY0;
    float   worldX1;
    float   worldY1;
    int     screenX0;
    int     screenY0;
    int     screenX1;
    int     screenY1;
    uint16_t depthLayer;

    RenderRoadConnection()
        : worldX0(0), worldY0(0)
        , worldX1(0), worldY1(0)
        , screenX0(0), screenY0(0)
        , screenX1(0), screenY1(0)
        , depthLayer(0) {}
};

} // namespace Scene
