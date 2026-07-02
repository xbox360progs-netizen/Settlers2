#include "stdafx.h"
#include "BuildingRenderer.h"
#include "../Shared/RenderFrame.h"
#include "../Shared/RenderBuilding.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/SpriteAtlas.h"

namespace Scene {

BuildingRenderer::BuildingRenderer()
    : m_buildingsAtlas(NULL)
    , m_buildingsSlot(0)
    , m_flagSpriteIdx(-1)
    , m_flagSpriteCached(false)
    , m_constrSpriteIdx(-1)
    , m_constrSpriteCached(false)
{
}

void BuildingRenderer::SetAtlases(
    SpriteAtlas* buildingsAtlas,
    int buildingsTextureSlot)
{
    m_buildingsAtlas = buildingsAtlas;
    m_buildingsSlot = buildingsTextureSlot;
    m_flagSpriteCached = false;
    m_constrSpriteCached = false;
}

int BuildingRenderer::ResolveSpriteIndex(const RenderBuilding& b)
{
    const BuildingVisual& v = b.visual;
    if (v.kind == 0) {
        if (!m_flagSpriteCached && m_buildingsAtlas) {
            m_flagSpriteIdx = (int)m_buildingsAtlas->GetIndex("flag");
            m_flagSpriteCached = true;
        }
        return m_flagSpriteIdx;
    }

    if (v.kind == 2) {
        if (!m_constrSpriteCached && m_buildingsAtlas) {
            m_constrSpriteIdx = (int)m_buildingsAtlas->GetIndex("construction");
            if (m_constrSpriteIdx < 0) m_constrSpriteIdx = (int)m_buildingsAtlas->GetIndex("Construction");
            if (m_constrSpriteIdx < 0) m_constrSpriteIdx = (int)m_buildingsAtlas->GetIndex("ConstructionSite");
            m_constrSpriteCached = true;
        }
        return m_constrSpriteIdx;
    }

    // kind == 1: no sprite rendered by BuildingRenderer (handled by terrain tile layer)
    return -1;
}

void BuildingRenderer::Render(
    RenderCommandBuffer& buffer,
    const RenderFrame& frame)
{
    if (!m_buildingsAtlas || frame.buildings.empty()) return;

    for (size_t i = 0; i < frame.buildings.size(); ++i) {
        const RenderBuilding& b = frame.buildings[i];
        const BuildingVisual& v = b.visual;

        if (v.kind == 0 || v.kind == 2) {
            int spriteIdx = ResolveSpriteIndex(b);
            if (spriteIdx < 0) continue;

            const SpriteRegion* r = m_buildingsAtlas->GetRegion(spriteIdx);
            if (!r) continue;

            buffer.PushSprite(
                b.transform.screenX - (int)(r->pivotX + 0.5f),
                b.transform.screenY - (int)(r->pivotY + 0.5f),
                (float)r->width, (float)r->height,
                r->u0, r->v0, r->u1, r->v1,
                m_buildingsSlot,
                static_cast<WORD>(b.transform.depthLayer),
                v.color);
            continue;
        }

        // kind == 1: state overlays for completed buildings
        if (v.kind == 1) {
            RenderStateOverlay(buffer, b.transform, v, static_cast<WORD>(b.transform.depthLayer));
        }
    }
}

void BuildingRenderer::RenderStateOverlay(
    RenderCommandBuffer& buffer,
    const RenderTransform& t,
    const BuildingVisual& v,
    WORD buildingDepth)
{
    // FSM state indicator: small colored rect above building.
    // HasWorker indicator: green dot if worker present.
    uint32_t stateColor = 0x00808080;  // Idle → semi-transparent gray

    switch (v.fsmState) {
        case 1: stateColor = 0xFF00FF00; break; // Producing → green
        case 2: stateColor = 0xFFFF0000; break; // OutputFull → red
        default: break;
    }

    float overlaySize = 6.0f;
    int ox1 = t.screenX - (int)(overlaySize * 0.5f);
    int oy1 = t.screenY - 48 - (int)(overlaySize * 0.5f); // fixed offset above building

    buffer.PushSprite(
        ox1, oy1, overlaySize, overlaySize,
        0.0f, 0.0f, 1.0f, 1.0f,
        0,
        static_cast<WORD>(buildingDepth + 5),
        stateColor);
}

} // namespace Scene
