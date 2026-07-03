#include "stdafx.h"
#include "BackgroundPass.h"
#include "Shared/RenderFrame.h"
#include "Rendering/RenderCommandBuffer.h"
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/RenderLayers.h"

namespace Scene {

BackgroundPass::BackgroundPass()
    : m_textureLoaded(false)
    , m_textureSlot(0)
{
}

void BackgroundPass::Execute(const RenderFrame& frame, const RenderContext& context,
                              RenderCommandBuffer& buffer)
{
    TextureRegistry& reg = TextureRegistry::instance();
    LPDIRECT3DTEXTURE9 bgTex = reg.getTextureOrLoad("background_game");
    if (!bgTex) return;

    if (!m_textureLoaded) {
        m_textureLoaded = true;
    }

    // The background is a full-screen sprite at the bottom of the depth order.
    // Push as a full-screen quad: position (0,0), size (1280,720).
    {
        // Background uses a separate texture slot. The texture was bound
        // externally (or can be bound here in a real pass).
        // For now, push a sprite — the slot texture must be bound in GameRenderer's
        // texture binding section, which happens before Execute().

        // We push a full-screen quad. Since PushSprite expects a texture slot
        // and the background texture is bound there, this renders full-screen.
        buffer.PushSprite(
            0, 0,
            1280.0f, 720.0f,
            0, 0, 1, 1,
            m_textureSlot, 0, 0xFFFFFFFF,
            0xFFFF, 0xFF, LAYER_EFFECTS
        );
    }
}

} // namespace Scene
