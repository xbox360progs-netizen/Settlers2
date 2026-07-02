#pragma once
#include "../Rendering/RenderPass.h"

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders the world-space cursor at the projected cursor tile position.
// Reads RenderFrame.cursor (screen coords pre-computed by ProjectionSystem)
// and pushes a single screen-space sprite to CommandBuffer.
// Caches the "cursor" sprite region from the UI atlas.
class CursorPass : public RenderPass {
public:
    CursorPass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    // Set the UI cursor texture slot (called by GameRenderer each frame).
    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    bool        m_atlasLoaded;
    float       m_u0, m_v0, m_u1, m_v1;
    float       m_w, m_h;
    float       m_pivotX, m_pivotY;

    void LoadAtlas();

    uint16_t m_textureSlot;

};

} // namespace Scene
