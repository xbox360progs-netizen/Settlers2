#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders ground resource icons + amount text overlays.
// Reads RenderFrame.groundResources (world coords, pre-projected by ProjectionSystem)
// and pushes screen-space sprites + text to CommandBuffer.
class GroundResourcePass : public RenderPass {
public:
    GroundResourcePass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    struct IconSprite {
        float u0, v0, u1, v1;
        float w, h;
        float pivotX, pivotY;
        bool  valid;
    };

    bool       m_atlasLoaded;
    uint16_t   m_textureSlot;
    IconSprite m_woodIcon;

    void LoadAtlas();
};

} // namespace Scene
