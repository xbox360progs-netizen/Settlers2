#pragma once
#include <stdint.h>

namespace Scene {

// Scene-side render command DTO. Represents a single sprite draw call
// from the perspective of scene renderers (SettlerRenderer, BuildingRenderer,
// TerrainPass). Converted to Graphics::RenderCommand in
// RenderCommandBuffer::SubmitToQueue.
//
// x,y are always screen pixels after ProjectionSystem runs.
// shaderId discriminates between screen-space (SHADER_WORLD_SCREEN,
// no VP transform) and world-space (SHADER_TERRAIN, uses VP matrix)
// rendering in the graphics backend.
struct RenderCommand {
    float x, y;              // position (screen or world depending on shaderId)
    float width, height;     // sprite dimensions
    float u0, v0, u1, v1;    // UV coords from atlas region
    uint16_t textureSlot;    // bound texture slot
    uint16_t depth;          // draw depth (sort key component)
    uint16_t shaderId;       // SHADER_WORLD_SCREEN, SHADER_TERRAIN, etc.
    uint8_t  blendMode;      // 0=opaque, 1=alpha blend
    uint8_t  layer;          // LAYER_WORLD, LAYER_TERRAIN, etc.
    uint32_t color;          // 0xFFFFFFFF = opaque white
};

}
