#include "stdafx.h"
#include "HuntingSpotPass.h"
#include "RenderOverlayMarker.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../Graphics/SpriteAtlas.h"
#include "../../Graphics/TextManager.h"
#include "../../Graphics/RenderLayers.h"
#include <cstdio>

namespace Scene {

HuntingSpotPass::HuntingSpotPass(TextManager* textManager)
    : m_atlasLoaded(false)
    , m_textureSlot(0)
    , m_textManager(textManager)
{
}

void HuntingSpotPass::LoadAtlas()
{
    TextureRegistry& reg = TextureRegistry::instance();
    reg.getTextureOrLoad("Icon");
    std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
    if (!iconAtlas) return;

    uint32_t deerIdx = iconAtlas->GetIndex("r_deer");
    if (deerIdx != 0xFFFFFFFF) {
        const SpriteRegion* r = iconAtlas->GetRegion(deerIdx);
        if (r) {
            m_deerSprite.u0 = r->u0; m_deerSprite.v0 = r->v0;
            m_deerSprite.u1 = r->u1; m_deerSprite.v1 = r->v1;
            m_deerSprite.w  = 20.0f;
            m_deerSprite.h  = 20.0f;
        }
    }

    m_atlasLoaded = true;
}

void HuntingSpotPass::Execute(
    const RenderFrame& frame, const RenderContext& context,
    RenderCommandBuffer& buffer)
{
    // Find hunting spot markers in the overlay list.
    bool hasAny = false;
    for (size_t i = 0; i < frame.overlays.size(); ++i) {
        if (frame.overlays[i].markerType == OVERLAY_MARKER_HUNTING_SPOT) {
            hasAny = true;
            break;
        }
    }
    if (!hasAny) return;

    if (!m_atlasLoaded) {
        LoadAtlas();
        if (!m_atlasLoaded) return;
    }

    for (size_t i = 0; i < frame.overlays.size(); ++i) {
        const RenderOverlayMarker& m = frame.overlays[i];
        if (m.markerType != OVERLAY_MARKER_HUNTING_SPOT) continue;

        int sx = m.transform.screenX;
        int sy = m.transform.screenY;
        uint16_t depth = static_cast<uint16_t>(m.transform.depthLayer);

        // Deer icon.
        if (m_deerSprite.w > 0.0f) {
            buffer.PushSprite(
                sx - m_deerSprite.w * 0.5f, sy - m_deerSprite.h,
                m_deerSprite.w, m_deerSprite.h,
                m_deerSprite.u0, m_deerSprite.v0,
                m_deerSprite.u1, m_deerSprite.v1,
                m_textureSlot, depth, D3DCOLOR_ARGB(200, 255, 255, 255),
                0xFFFF, 0xFF, LAYER_FOREGROUND);
        }

        // Amount text.
        if (m_textManager && m.amount > 0) {
            char buf[8];
            _snprintf(buf, sizeof(buf), "%d", m.amount);
            m_textManager->DrawTextToWorld(
                buf, m.transform.worldX, m.transform.worldY - 28.0f,
                D3DCOLOR_ARGB(255, 255, 255, 0), 0.07f);
        }
    }
}

} // namespace Scene
