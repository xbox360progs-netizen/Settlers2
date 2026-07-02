#pragma once

namespace Graphics {
    class SpriteAtlas;
}

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;

// Pure rendering: resolves building/flag sprites from DTO fields, pushes to CommandBuffer.
// Knows nothing about simulation or FlagManager.
class BuildingRenderer {
public:
    BuildingRenderer();

    void SetAtlases(
        Graphics::SpriteAtlas* buildingsAtlas,
        int buildingsTextureSlot
    );

    void Render(
        RenderCommandBuffer& buffer,
        const RenderFrame& frame
    );

private:
    int ResolveSpriteIndex(const struct RenderBuilding& b) const;

    Graphics::SpriteAtlas* m_buildingsAtlas;
    int m_buildingsSlot;
    int m_flagSpriteIdx;     // cached index of "flag" sprite
    bool m_flagSpriteCached;
};

} // namespace Scene
