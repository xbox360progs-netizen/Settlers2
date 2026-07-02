#pragma once
#include <vector>
#include <stdint.h>
#include "RenderCommand.h"

namespace Graphics {
    class RenderQueue;
}

namespace Scene {

// Collects render commands from all scene renderers (SettlerRenderer,
// BuildingRenderer, TerrainPass, etc.) and submits them to the graphics
// RenderQueue in a single batch. Sorts by depth internally.
class RenderCommandBuffer {
public:
    RenderCommandBuffer();
    ~RenderCommandBuffer();

    // Push a screen-space projected sprite.
    // Default shader/blend/layer are for standard entity rendering
    // (SHADER_WORLD_SCREEN, alpha blend, LAYER_WORLD). Override for
    // terrain (LAYER_TERRAIN, different blend modes).
    void PushSprite(int x, int y, float w, float h,
                    float u0, float v0, float u1, float v1,
                    uint16_t textureSlot, uint16_t depth,
                    uint32_t color = 0xFFFFFFFF,
                    uint16_t shaderId = 0xFFFF,      // 0xFFFF = use default (SHADER_WORLD_SCREEN)
                    uint8_t  blendMode = 0xFF,        // 0xFF = use default (1 = alpha)
                    uint8_t  layer = 0xFF);           // 0xFF = use default (LAYER_WORLD)

    // Clear all buffered commands (call once per frame before building).
    void Clear();

    // Convert buffered commands to Graphics::RenderCommand and submit
    // to the render queue. Sorts by depth internally.
    void SubmitToQueue(Graphics::RenderQueue* queue);

    // Future: shadow pass for projected entities
    // void PushShadow(...);

    // Future: overlay pass (selection highlights, debug overlays)
    // void PushOverlay(...);

private:
    std::vector<RenderCommand> m_commands;
};

}
