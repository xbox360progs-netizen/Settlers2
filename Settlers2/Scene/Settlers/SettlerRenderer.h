#pragma once

namespace Graphics {
    class SpriteAtlas;
    class SpriteRenderer;
}

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderSettler;

// Pure rendering: resolves sprite from DTO fields, pushes to CommandBuffer.
// Knows nothing about simulation, carriers, builders, or roads.
// All positional data arrives pre-computed via RenderFrame DTOs.
class SettlerRenderer {
public:
    SettlerRenderer();

    // Must be called once per frame before Render() with the correct atlases.
    void SetAtlases(
        Graphics::SpriteAtlas* unitsAtlas,
        Graphics::SpriteAtlas* iconAtlas,
        int unitTextureSlot
    );

    // Render all settlers from the frame snapshot. No simulation reads.
    void Render(
        RenderCommandBuffer& buffer,
        const RenderFrame& frame
    );

private:
    // Resolve sprite index from DTO fields (no simulation state needed).
    int ResolveSpriteIndex(const RenderSettler& s) const;

    Graphics::SpriteAtlas* m_unitsAtlas;
    Graphics::SpriteAtlas* m_iconAtlas;
    int m_unitSlot;
};

} // namespace Scene
