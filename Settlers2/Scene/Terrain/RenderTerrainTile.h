#pragma once
#include <stdint.h>

namespace Scene {

// Per-tile render data. worldX/worldY are set by TerrainPresentationSystem;
// screenX/screenY are set by ProjectionSystem in the same pass as settlers
// and buildings. The render path (TerrainPass → TileRenderer) reads only
// screen coords — no camera or world→screen math in the renderer.
struct RenderTerrainTile {
    float worldX, worldY;        // world position (input to ProjectionSystem)
    float screenX, screenY;      // screen position (output from ProjectionSystem)
    float width, height;         // sprite dimensions (from region or default)
    float u0, v0, u1, v1;        // UV coords from atlas region
    uint16_t depth;              // draw depth (pre-computed in Presentation)
    uint8_t blendMode;           // 0=opaque, 1=alpha
    uint8_t layerType;           // 0=Ground, 1=Roads, 2=Objects, 3=Buildings
    char atlasName[32];          // texture atlas name (resolved to slot at render time)
};

}
