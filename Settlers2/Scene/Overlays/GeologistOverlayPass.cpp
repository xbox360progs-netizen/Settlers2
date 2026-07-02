#include "stdafx.h"
#include "GeologistOverlayPass.h"
#include "RenderOverlayMarker.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../Graphics/SpriteAtlas.h"
#include "../../Graphics/RenderLayers.h"
#include "../../World/ResourceNode.h"

namespace Scene {

GeologistOverlayPass::GeologistOverlayPass()
    : m_atlasLoaded(false)
    , m_textureSlot(0)
{
}

void GeologistOverlayPass::LoadAtlas()
{
    TextureRegistry& reg = TextureRegistry::instance();
    reg.getTextureOrLoad("Icon");
    std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
    if (!iconAtlas) return;

    // Mountain highlight — use a small bright yellow quad.
    m_mountainSprite.u0 = 0.5f; m_mountainSprite.v0 = 0.5f;
    m_mountainSprite.u1 = 0.5001f; m_mountainSprite.v1 = 0.5001f;
    m_mountainSprite.w  = 60.0f;
    m_mountainSprite.h  = 30.0f;

    // Working indicator — icon_geologist_work from Icon atlas.
    {
        uint32_t idx = iconAtlas->GetIndex("icon_geologist_work");
        if (idx != 0xFFFFFFFF) {
            const SpriteRegion* r = iconAtlas->GetRegion(idx);
            if (r) {
                m_workingSprite.u0 = r->u0; m_workingSprite.v0 = r->v0;
                m_workingSprite.u1 = r->u1; m_workingSprite.v1 = r->v1;
                m_workingSprite.w  = static_cast<float>(r->width);
                m_workingSprite.h  = static_cast<float>(r->height);
            }
        }
    }

    // Pre-cache deposit icons for all known resource types.
    for (int rt = 0; rt < 32; ++rt) {
        World::ResourceType type = static_cast<World::ResourceType>(rt);
        const char* name = World::ResourceTypeToDepositIconName(type);
        if (!name || name[0] == '\0') continue;

        uint32_t idx = iconAtlas->GetIndex(name);
        if (idx == 0xFFFFFFFF) continue;

        const SpriteRegion* r = iconAtlas->GetRegion(idx);
        if (!r) continue;

        DepositSprite ds;
        ds.resourceType = static_cast<uint8_t>(type);
        ds.sprite.u0 = r->u0; ds.sprite.v0 = r->v0;
        ds.sprite.u1 = r->u1; ds.sprite.v1 = r->v1;
        ds.sprite.w  = static_cast<float>(r->width) * 0.8f;
        ds.sprite.h  = static_cast<float>(r->height) * 0.8f;
        m_depositCache.push_back(ds);
    }

    m_atlasLoaded = true;
}

const GeologistOverlayPass::SpriteInfo* GeologistOverlayPass::GetDepositSprite(
    uint8_t resourceType)
{
    for (size_t i = 0; i < m_depositCache.size(); ++i) {
        if (m_depositCache[i].resourceType == resourceType)
            return &m_depositCache[i].sprite;
    }
    return NULL;
}

void GeologistOverlayPass::Execute(
    const RenderFrame& frame, const RenderContext& context, RenderCommandBuffer& buffer)
{
    const std::vector<RenderOverlayMarker>& markers = frame.overlays;
    if (markers.empty()) return;

    if (!m_atlasLoaded) {
        LoadAtlas();
        if (!m_atlasLoaded) return;
    }

    for (size_t i = 0; i < markers.size(); ++i) {
        const RenderOverlayMarker& m = markers[i];
        int sx = m.transform.screenX;
        int sy = m.transform.screenY;
        uint16_t depth = static_cast<uint16_t>(m.transform.depthLayer);

        switch (m.markerType) {
        case OVERLAY_MARKER_MOUNTAIN:
            buffer.PushSprite(
                sx - 30, sy - 15,
                m_mountainSprite.w, m_mountainSprite.h,
                m_mountainSprite.u0, m_mountainSprite.v0,
                m_mountainSprite.u1, m_mountainSprite.v1,
                m_textureSlot, depth, 0x50FFFF00,
                0xFFFF, 0xFF, LAYER_EFFECTS);
            break;

        case OVERLAY_MARKER_DEPOSIT: {
            const SpriteInfo* si = GetDepositSprite(m.resourceType);
            if (si) {
                buffer.PushSprite(
                    sx - si->w * 0.5f, sy - si->h * 0.5f,
                    si->w, si->h,
                    si->u0, si->v0, si->u1, si->v1,
                    m_textureSlot, depth, 0xDCFFFFFF,
                    0xFFFF, 0xFF, LAYER_EFFECTS);
            } else {
                // Fallback: small colored quad.
                uint32_t fallbackColor = 0xC8FFFF00;
                switch (static_cast<World::ResourceType>(m.resourceType)) {
                case World::ResourceType_Coal:    fallbackColor = 0xC8505050; break;
                case World::ResourceType_IronOre: fallbackColor = 0xC8B46632; break;
                case World::ResourceType_GoldOre: fallbackColor = 0xC8FFD700; break;
                case World::ResourceType_Stone:   fallbackColor = 0xC8969696; break;
                case World::ResourceType_Marble:  fallbackColor = 0xC8C8B4DC; break;
                case World::ResourceType_Granite: fallbackColor = 0xC8825A46; break;
                default: break;
                }
                buffer.PushSprite(
                    sx - 12, sy - 12, 24, 24,
                    0.5f, 0.5f, 0.5001f, 0.5001f,
                    m_textureSlot, depth, fallbackColor,
                    0xFFFF, 0xFF, LAYER_EFFECTS);
            }
            break;
        }

        case OVERLAY_MARKER_WORKING:
            buffer.PushSprite(
                sx - m_workingSprite.w * 0.5f, sy - m_workingSprite.h * 0.5f,
                m_workingSprite.w, m_workingSprite.h,
                m_workingSprite.u0, m_workingSprite.v0,
                m_workingSprite.u1, m_workingSprite.v1,
                m_textureSlot, depth, 0xFFFFFFFF,
                0xFFFF, 0xFF, LAYER_EFFECTS);
            break;
        }
    }
}

} // namespace Scene
