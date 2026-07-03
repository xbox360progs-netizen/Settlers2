#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders committed road connection quads (E/W quads between adjacent tiles).
// Reads RenderFrame.roadConnections with screen coords pre-computed by ProjectionSystem.
// Caches the "street_1" sprite from the streets atlas.
class RoadConnectionPass : public RenderPass {
public:
    RoadConnectionPass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    struct ConnectionSprite {
        float u0, v0, u1, v1;
        float w, h;
        float pivotX, pivotY;
    };

    bool             m_atlasLoaded;
    uint16_t         m_textureSlot;
    ConnectionSprite m_sprite;

    void LoadAtlas();
};

} // namespace Scene
