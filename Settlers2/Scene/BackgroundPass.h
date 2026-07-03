#pragma once
#include "Rendering/RenderPass.h"
#include <stdint.h>

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders the full-screen background sprite.
// Pure infrastructure pass — no simulation dependencies, no DTO.
// Uses TextureRegistry to load and bind "background_game" texture.
class BackgroundPass : public RenderPass {
public:
    BackgroundPass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    bool     m_textureLoaded;
    uint16_t m_textureSlot;
};

} // namespace Scene
