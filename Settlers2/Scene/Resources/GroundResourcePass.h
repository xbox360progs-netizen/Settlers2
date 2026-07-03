#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

class TextManager;

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders ground resource icons + amount text overlays.
// Reads RenderFrame.groundResources with pre-projected screen coords
// (computed by ProjectionSystem including textScreenX/textScreenY).
// Pushes screen-space sprites to CommandBuffer and renders text via TextManager.
class GroundResourcePass : public RenderPass {
public:
    explicit GroundResourcePass(TextManager* textManager);
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

    TextManager* m_textManager;
    bool         m_atlasLoaded;
    uint16_t     m_textureSlot;
    IconSprite   m_woodIcon;

    void LoadAtlas();
};

} // namespace Scene
