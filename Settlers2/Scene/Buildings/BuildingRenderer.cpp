#include "stdafx.h"
#include "BuildingRenderer.h"
#include "../Shared/RenderFrame.h"
#include "../Shared/RenderBuilding.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/SpriteAtlas.h"
#include "../TextureSlots.h"
#include "../BuildingPlacement.h"

namespace Scene {

BuildingRenderer::BuildingRenderer()
    : m_buildingsAtlas(NULL)
    , m_buildingsSlot(SLOT_BUILDINGS_HIGHLIGHT)
    , m_flagSpriteIdx(-1)
    , m_flagSpriteCached(false)
{
}

void BuildingRenderer::SetAtlases(
    Graphics::SpriteAtlas* buildingsAtlas,
    int buildingsTextureSlot)
{
    m_buildingsAtlas = buildingsAtlas;
    m_buildingsSlot = buildingsTextureSlot;
    m_flagSpriteCached = false;
}

int BuildingRenderer::ResolveSpriteIndex(const RenderBuilding& b) const
{
    const BuildingVisual& v = b.visual;
    if (v.kind == 0) {
        // Flag sprite
        if (!m_flagSpriteCached && m_buildingsAtlas) {
            m_flagSpriteIdx = (int)m_buildingsAtlas->GetIndex("flag");
            m_flagSpriteCached = true;
        }
        return m_flagSpriteIdx;
    }

    // Building sprite
    const char* spriteName = BuildingPlacementManager::GetBuildingSpriteName(
        static_cast<World::BuildingType>(v.buildingType));
    if (!spriteName || !spriteName[0]) return -1;

    if (v.depleted && m_buildingsAtlas) {
        std::string depletedName = std::string(spriteName) + "_depleted";
        int idx = (int)m_buildingsAtlas->GetIndex(depletedName.c_str());
        if (idx >= 0) return idx;
    }

    if (m_buildingsAtlas) {
        return (int)m_buildingsAtlas->GetIndex(spriteName);
    }
    return -1;
}

void BuildingRenderer::Render(
    RenderCommandBuffer& buffer,
    const RenderFrame& frame)
{
    if (!m_buildingsAtlas || frame.buildings.empty()) return;

    for (size_t i = 0; i < frame.buildings.size(); ++i) {
        const RenderBuilding& b = frame.buildings[i];
        const RenderTransform& t = b.transform;

        int spriteIdx = ResolveSpriteIndex(b);
        if (spriteIdx < 0) continue;

        const SpriteRegion* r = m_buildingsAtlas->GetRegion(spriteIdx);
        if (!r) continue;

        WORD depth = static_cast<WORD>(t.depthLayer);

        uint32_t color = b.visual.color;
        buffer.PushSprite(
            t.screenX - (int)(r->pivotX + 0.5f),
            t.screenY - (int)(r->pivotY + 0.5f),
            (float)r->width, (float)r->height,
            r->u0, r->v0, r->u1, r->v1,
            m_buildingsSlot, depth, color);
    }
}

} // namespace Scene
