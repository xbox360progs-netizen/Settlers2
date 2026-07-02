#pragma once
#include <vector>
#include <stdint.h>
#include "../Settlers/RenderSettler.h"
#include "RenderBuilding.h"
#include "../Terrain/RenderTerrainTile.h"
#include "../Cursor/RenderCursor.h"
#include "../Flags/RenderFlagResource.h"
#include "../Wildlife/RenderWildlife.h"
#include "../Placement/RenderPlacementPreview.h"
#include "../Roads/RenderRoadPreview.h"
#include "../Overlays/RenderOverlayMarker.h"
#include "../UI/RenderUiFrame.h"

namespace Scene {

// Immutable per-frame snapshot of everything that needs rendering.
// Produced by PresentationSystem (Core1), consumed by GameRenderer (Core0).
// No pointers to simulation state — only POD data.
struct RenderFrame {
    uint32_t frameId;                  // incrementing frame counter
    uint32_t simulationTick;           // simulation tick at capture time

    std::vector<RenderSettler> settlers;
    std::vector<RenderBuilding> buildings;
    std::vector<RenderTerrainTile> terrain;
    RenderCursor                  cursor;
    std::vector<RenderFlagResource> flagResources;
    std::vector<RenderWildlife>    wildlife;
    std::vector<RenderPlacementPreview> preview;
    std::vector<RenderRoadSegment> roadPreview;
    std::vector<RenderOverlayMarker> overlays;
    RenderUiFrame ui;

    // Future: std::vector<RenderEffect> effects;

    RenderFrame() : frameId(0), simulationTick(0) {}
};

} // namespace Scene
