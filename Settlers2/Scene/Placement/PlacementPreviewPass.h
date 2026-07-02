#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders building/flag placement preview ghost (green/red tinted sprite).
// Reads RenderFrame.preview (screen coords pre-computed by ProjectionSystem)
// and pushes a screen-space sprite to CommandBuffer.
// Caches the currently needed sprite from the Buildings atlas on demand.
class PlacementPreviewPass : public RenderPass {
public:
    PlacementPreviewPass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    struct BuildingSprite {
        float u0, v0, u1, v1;
        float w, h;
        float pivotX, pivotY;
        bool  valid;
    };

    bool     m_atlasLoaded;
    uint16_t m_textureSlot;
    BuildingSprite m_spriteCache[32];  // indexed by BuildingType (0 = Building_None unused)
    bool     m_spriteLoaded[32];       // whether each entry was resolved

    // Resolve a single BuildingType entry in the cache.
    void CacheSprite(uint8_t buildingType);
    // Look up sprite name for a building type (mirrors BuildingPlacementManager::GetBuildingSpriteName).
    const char* SpriteNameForType(uint8_t buildingType) const;
};

} // namespace Scene
