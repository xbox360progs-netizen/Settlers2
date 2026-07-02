#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders road placement preview (path tiles + connection quads + neighbor hints).
// Reads RenderFrame.roadPreview with screen coords pre-computed by ProjectionSystem.
// Pushes screen-space sprites to CommandBuffer.
// Caches the "street_1" sprite from the streets atlas and computes flag alignment once.
class RoadPreviewPass : public RenderPass {
public:
    RoadPreviewPass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    struct StreetSprite {
        float u0, v0, u1, v1;
        float w, h;
        float pivotX, pivotY;
    };

    bool        m_atlasLoaded;
    uint16_t    m_textureSlot;
    StreetSprite m_tileSprite;    // cached "street_1" sprite
    StreetSprite m_connectionSprite; // same texture, used for horizontal quads
    float       m_alignOffsetX;  // flag-to-street pivot alignment offset

    void LoadAtlas();
};

} // namespace Scene
