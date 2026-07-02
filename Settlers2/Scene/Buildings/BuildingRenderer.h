#pragma once

class SpriteAtlas;

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderBuilding;
struct BuildingVisual;
struct RenderTransform;

// Pure rendering: resolves building/flag sprites from DTO fields, pushes to CommandBuffer.
// Knows nothing about simulation or FlagManager.
// Flags (kind=0) → renders flag pole sprite.
// Buildings (kind=1) → renders state overlays only (building sprite handled by terrain tile layer).
// Construction sites (kind=2) → renders scaffolding sprite + builder indicator.
class BuildingRenderer {
public:
    BuildingRenderer();

    void SetAtlases(
        SpriteAtlas* buildingsAtlas,
        int buildingsTextureSlot
    );

    void Render(
        RenderCommandBuffer& buffer,
        const RenderFrame& frame
    );

private:
    int ResolveSpriteIndex(const struct RenderBuilding& b);

    void RenderStateOverlay(
        RenderCommandBuffer& buffer,
        const struct RenderTransform& t,
        const struct BuildingVisual& v,
        unsigned short buildingDepth);

    SpriteAtlas* m_buildingsAtlas;
    int m_buildingsSlot;
    int m_flagSpriteIdx;
    bool m_flagSpriteCached;
    int m_constrSpriteIdx;
    bool m_constrSpriteCached;
};

} // namespace Scene
