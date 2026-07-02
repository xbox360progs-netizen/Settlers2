#pragma once

class SpriteAtlas;

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;

// DEPRECATED: WorkerPass + WorkerRenderer handle all worker rendering.
// Kept only because SettlerPass is still registered in RenderGraph.
class SettlerRenderer {
public:
    SettlerRenderer();

    void SetAtlases(
        SpriteAtlas* unitsAtlas,
        SpriteAtlas* iconAtlas,
        int unitTextureSlot
    );

    void Render(
        RenderCommandBuffer& buffer,
        const RenderFrame& frame
    );

private:
    SpriteAtlas* m_unitsAtlas;
    SpriteAtlas* m_iconAtlas;
    int m_unitSlot;
};

} // namespace Scene
